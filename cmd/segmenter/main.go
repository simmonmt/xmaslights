package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"html/template"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"path"
	"path/filepath"
	"time"

	"github.com/simmonmt/xmaslights/lib/go/ddp"
)

var (
	docRoot        = flag.String("docroot", "", "Root directory for files")
	indexPath      = flag.String("index", "", "index.html file")
	minPixel       = flag.Int("min_light", -1, "Minimum pixel number")
	maxPixel       = flag.Int("max_light", -1, "Maximum pixel number")
	port           = flag.Int("port", 8080, "Port number")
	onColor        = flag.Uint("on_color", 0x00ff00, "Color to use for ON")
	offColor       = flag.Uint("off_color", 0xff0000, "Color to use for OFF")
	blinkColor     = flag.Uint("blink_color", 0x0000ff, "Color to use for the current light")
	chaseColor     = flag.Uint("chase_color", 0x00ff00, "Color to use for the chaser")
	ddpAddress     = flag.String("ddp", "", "DDP controller host[:port]")
	verboseDDPConn = flag.Bool("verbose_ddp", false, "Use verbose DDP connection")
	savePath       = flag.String("save_path", "", "File where /save messages should be written")
	seedPath       = flag.String("seed_path", "", "File from which initial state is to be read")

	ddpPeriod  = 100 * time.Millisecond // How frequently to send DDP updates
	animPeriod = 100 * time.Millisecond
)

type Range struct {
	From int `json:"from"`
	To   int `json:"to"`
}

type TemplateArgs struct {
	MinLight int
	MaxLight int
	Seed     []Range
}

type UpdateMetadata struct {
	CurLight int
	Mode     string
}

type UpdateRequest struct {
	Metadata UpdateMetadata
	OnRanges []Range
}

func readSeed(path string) ([]Range, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, fmt.Errorf("failed to open %v: %v", path, err)
	}

	scan := bufio.NewScanner(f)
	var last string
	var warned bool
	for scan.Scan() {
		if last != "" && !warned {
			log.Printf("WARNING: seed file contains multiple lines;" +
				"using the last one")
			warned = true
		}
		last = scan.Text()
	}

	if last == "" {
		return nil, fmt.Errorf("no seed line found")
	}

	ranges := []Range{}
	if err := json.Unmarshal([]byte(last), &ranges); err != nil {
		return nil, fmt.Errorf("failed to parse seed: %v", err)
	}

	return ranges, nil
}

type DDPState struct {
	conn      *ddp.DDPConn
	addr      *net.UDPAddr
	numPixels int
	data      []byte
}

func newDDPState(conn *ddp.DDPConn, addr *net.UDPAddr, numPixels int) *DDPState {
	return &DDPState{
		conn:      conn,
		addr:      addr,
		numPixels: numPixels,
		data:      make([]byte, numPixels*3),
	}
}

func updateStates(ranges []Range, states []bool) {
	// We could walk through both at the same time but that seems tedious
	// and fiddly and this way I don't have to write a unit test.

	for i := range states {
		states[i] = false
	}

	for _, r := range ranges {
		for i := r.From; i <= r.To; i++ {
			if i > 0 && i < len(states) {
				states[i] = true
			}
		}
	}
}

func updateDdp(ddpData []byte, minPixel int, states []bool, curPixel int, blinkOn bool, chaseOffset int) {
	maxDdp := len(ddpData)/3 - 1

	set := func(num int, color uint) {
		if num < 0 || num > maxDdp {
			return
		}

		ddpData[num*3+0] = byte((color >> 16) & 0xff)
		ddpData[num*3+1] = byte((color >> 8) & 0xff)
		ddpData[num*3+2] = byte(color & 0xff)
	}

	for i, on := range states {
		if i < minPixel {
			set(i, 0)
		} else if on {
			set(i, *onColor)
		} else {
			set(i, *offColor)
		}
	}
	if blinkOn {
		set(curPixel, *blinkColor)
	}

	if chaseOffset != 0 {
		set(curPixel-chaseOffset, *chaseColor)
		set(curPixel+chaseOffset, *chaseColor)
	}
}

func sendDdp(ddpState *DDPState) {
	ddpState.conn.SetPixels(ddpState.data, ddpState.addr)
}

type ActionMode int

const (
	MODE_UNKNOWN ActionMode = iota
	MODE_NAV
	MODE_ON
	MODE_OFF
	MODE_FIND
)

func parseMode(str string) ActionMode {
	switch str {
	case "NAV":
		return MODE_NAV
	case "ON":
		return MODE_ON
	case "OFF":
		return MODE_OFF
	case "FIND":
		return MODE_FIND
	default:
		return MODE_UNKNOWN
	}
}

type Chaser struct {
	Size   int
	offset int
	mode   ActionMode
}

func (c *Chaser) Inc() {
	offset := c.offset - 1
	if offset < 0 {
		offset = c.Size * 2
	}
	c.offset = offset
}

func (c *Chaser) SetMode(mode ActionMode) {
	c.mode = mode
}

func (c *Chaser) CurOffset() int {
	if c.mode == MODE_FIND {
		return c.offset / 2
	}
	return 0
}

type Blinker struct {
	state int
}

func (b *Blinker) Inc() {
	b.state = (b.state + 1) % 4
}

func (b *Blinker) CurState() bool {
	return b.state < 2
}

func ControlPixels(ddpConn *ddp.DDPConn, ddpAddr *net.UDPAddr, minPixel, maxPixel int, newUr chan *UpdateRequest, quit chan bool) {
	ddpState := newDDPState(ddpConn, ddpAddr, maxPixel+1)
	ddpTicker := time.NewTicker(ddpPeriod)

	curPixel := 0
	states := make([]bool, maxPixel+1)

	chaser := &Chaser{Size: 5}
	blinker := &Blinker{}

	animTicker := time.NewTicker(animPeriod)

	log.Print("pixel controller starting")
	for {
		select {
		case <-quit:
			log.Print("pixel controller exiting")
			return

		case ur := <-newUr:
			curPixel = ur.Metadata.CurLight
			chaser.SetMode(parseMode(ur.Metadata.Mode))
			updateStates(ur.OnRanges, states)
			updateDdp(ddpState.data, minPixel, states, curPixel,
				blinker.CurState(), chaser.CurOffset())

		case <-animTicker.C:
			chaser.Inc()
			blinker.Inc()

			updateDdp(ddpState.data, minPixel, states, curPixel,
				blinker.CurState(), chaser.CurOffset())

		case <-ddpTicker.C:
			sendDdp(ddpState)
		}
	}
}

func decodeUpdateRequest(r io.Reader) (*UpdateRequest, error) {
	setReq := &UpdateRequest{}
	if err := json.NewDecoder(r).Decode(setReq); err != nil {
		log.Printf("failed to decode request: %v", err)
		return nil, err
	}

	return setReq, nil
}

func setHandler(w http.ResponseWriter, req *http.Request, urChan chan *UpdateRequest) {
	setReq, err := decodeUpdateRequest(req.Body)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	urChan <- setReq
}

type Saver interface {
	Save(ranges []Range) error
}

type LogSaver struct{}

func (s *LogSaver) Save(ranges []Range) error {
	log.Printf("saving: %v", ranges)
	return nil
}

type FileSaver struct {
	f *os.File
}

func NewFileSaver(path string) (*FileSaver, error) {
	f, err := os.Create(path)
	if err != nil {
		return nil, err
	}

	return &FileSaver{f: f}, nil
}

func (s *FileSaver) Save(ranges []Range) error {
	enc, err := json.Marshal(ranges)
	if err != nil {
		return err
	}

	buf := &bytes.Buffer{}
	if err := json.Compact(buf, enc); err != nil {
		return err
	}
	buf.WriteByte('\n')

	if _, err := buf.WriteTo(s.f); err != nil {
		return err
	}

	if err := s.f.Sync(); err != nil {
		return err
	}

	log.Println("saved state")

	return nil
}

func saveHandler(w http.ResponseWriter, req *http.Request, s Saver) {
	updateReq, err := decodeUpdateRequest(req.Body)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	if err := s.Save(updateReq.OnRanges); err != nil {
		log.Printf("failed to save %v: %v", updateReq.OnRanges, err)
		return
	}
}

func main() {
	flag.Parse()

	if *docRoot == "" {
		log.Fatalf("--docroot is required")
	}
	if *indexPath == "" {
		log.Fatalf("--index is required")
	}
	if *minPixel == -1 {
		log.Fatalf("--min_light is required")
	}
	if *maxPixel == -1 {
		log.Fatalf("--max_light is required")
	}
	if *ddpAddress == "" {
		log.Fatal("--ddp is required")
	}

	seed := []Range{}
	if *seedPath != "" {
		var err error
		if seed, err = readSeed(*seedPath); err != nil {
			log.Fatalf("failed to read seed: %v", err)
		}
		log.Printf("using seed: %v\n", seed)
	}

	ddpAddr, err := net.ResolveUDPAddr("udp", ddp.MaybeAddDDPPort(*ddpAddress))
	if err != nil {
		log.Fatalf("failed to resolve controller: %v", err)
	}

	ddpConn, err := ddp.NewDDPConn(*verboseDDPConn)
	if err != nil {
		log.Fatalf("failed to make DDP connection: %v", err)
	}

	tmpl, err := template.ParseFiles(*indexPath)
	if err != nil {
		log.Fatalf("failed to parse index template: %v", err)
	}

	urChan := make(chan *UpdateRequest)
	quitChan := make(chan bool)

	var saver Saver
	if *savePath == "" {
		saver = &LogSaver{}
	} else {
		var err error
		saver, err = NewFileSaver(*savePath)
		if err != nil {
			log.Fatalf("failed to make file saver: %v", err)
		}
	}

	go ControlPixels(ddpConn, ddpAddr, *minPixel, *maxPixel, urChan, quitChan)

	rootHandler := func(w http.ResponseWriter, req *http.Request) {
		if err := tmpl.Execute(w, TemplateArgs{MinLight: *minPixel, MaxLight: *maxPixel, Seed: seed}); err != nil {
			log.Printf("failed to render template: %v", err)
			w.WriteHeader(http.StatusInternalServerError)
		}
	}

	http.HandleFunc("/index.html", rootHandler)
	http.HandleFunc("/{$}", rootHandler)

	http.HandleFunc("/set", func(w http.ResponseWriter, req *http.Request) {
		setHandler(w, req, urChan)
	})
	http.HandleFunc("/save", func(w http.ResponseWriter, req *http.Request) {
		saveHandler(w, req, saver)
	})

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		cleaned := filepath.Clean(req.URL.Path)
		http.ServeFile(w, req, path.Join(*docRoot, cleaned))
	})

	log.Printf("serving on port %v", *port)
	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", *port), nil))
}
