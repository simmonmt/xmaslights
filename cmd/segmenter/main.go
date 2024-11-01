package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"html/template"
	"io"
	"log"
	"net"
	"net/http"
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
	ddpAddress     = flag.String("ddp", "", "DDP controller host[:port]")
	verboseDDPConn = flag.Bool("verbose_ddp", false, "Use verbose DDP connection")

	ddpPeriod     = 100 * time.Millisecond // How frequently to send DDP updates
	curHalfPeriod = 500 * time.Millisecond
)

type TemplateArgs struct {
	MinLight int
	MaxLight int
}

type Range struct {
	From, To int
}

type SetRequest struct {
	CurLight int
	OnRanges []Range
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

func updateDdp(ddpData []byte, minPixel int, states []bool, curBlink int, blinkState bool) {
	set := func(num int, color uint) {
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
	if blinkState && curBlink >= 0 {
		set(curBlink, *blinkColor)
	}
}

func sendDdp(ddpState *DDPState) {
	ddpState.conn.SetPixels(ddpState.data, ddpState.addr)
}

func ControlPixels(ddpConn *ddp.DDPConn, ddpAddr *net.UDPAddr, minPixel, maxPixel int, newSr chan *SetRequest, quit chan bool) {
	ddpState := newDDPState(ddpConn, ddpAddr, maxPixel+1)
	ddpTicker := time.NewTicker(ddpPeriod)

	curLight := 0
	states := make([]bool, maxPixel+1)

	blinkState := false
	blinkTicker := time.NewTicker(curHalfPeriod)

	log.Print("pixel controller starting")
	for {
		select {
		case <-quit:
			log.Print("pixel controller exiting")
			return

		case sr := <-newSr:
			curLight = sr.CurLight
			updateStates(sr.OnRanges, states)
			updateDdp(ddpState.data, minPixel, states, curLight, blinkState)

		case <-blinkTicker.C:
			blinkState = !blinkState
			updateDdp(ddpState.data, minPixel, states, curLight, blinkState)

		case <-ddpTicker.C:
			sendDdp(ddpState)
		}
	}
}

func decodeSetRequest(r io.Reader) (*SetRequest, error) {
	setReq := &SetRequest{}
	if err := json.NewDecoder(r).Decode(setReq); err != nil {
		log.Printf("failed to decode request: %v", err)
		return nil, err
	}

	return setReq, nil
}

func setHandler(w http.ResponseWriter, req *http.Request, srChan chan *SetRequest) {
	setReq, err := decodeSetRequest(req.Body)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	srChan <- setReq
}

func saveHandler(w http.ResponseWriter, req *http.Request) {
	setReq, err := decodeSetRequest(req.Body)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	log.Printf("saving: %v", setReq.OnRanges)
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

	srChan := make(chan *SetRequest)
	quitChan := make(chan bool)

	go ControlPixels(ddpConn, ddpAddr, *minPixel, *maxPixel, srChan, quitChan)

	rootHandler := func(w http.ResponseWriter, req *http.Request) {
		if err := tmpl.Execute(w, TemplateArgs{MinLight: *minPixel, MaxLight: *maxPixel}); err != nil {
			log.Printf("failed to render template: %v", err)
			w.WriteHeader(http.StatusInternalServerError)
		}
	}

	http.HandleFunc("/index.html", rootHandler)
	http.HandleFunc("/{$}", rootHandler)

	http.HandleFunc("/set", func(w http.ResponseWriter, req *http.Request) {
		setHandler(w, req, srChan)
	})
	http.HandleFunc("/save", saveHandler)

	http.HandleFunc("/", func(w http.ResponseWriter, req *http.Request) {
		cleaned := filepath.Clean(req.URL.Path)
		http.ServeFile(w, req, path.Join(*docRoot, cleaned))
	})

	log.Printf("serving on port %v", *port)
	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", *port), nil))
}
