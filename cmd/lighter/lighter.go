package main

import (
	"flag"
	"fmt"
	"log"
	"net"
	"strconv"
	"time"

	"github.com/gdamore/tcell"
	"github.com/simmonmt/xmaslights/lib/go/ddp"
)

var (
	controllerAddress = flag.String("controller", "", "IP Address for the controller")
	numPixels         = flag.Int("num_pixels", 0, "Number of controlled pixels")
	verboseDDPConn    = flag.Bool("verbose_ddp", false, "Use verbose DDP connection")
)

func drawText(s tcell.Screen, x1, y1, x2, y2 int, style tcell.Style, text string) {
	row := y1
	col := x1
	for _, r := range []rune(text) {
		s.SetContent(col, row, r, nil, style)
		col++
		if col >= x2 {
			row++
			col = x1
		}
		if row > y2 {
			break
		}
	}

	for row <= y2 {
		for col <= x2 {
			s.SetContent(col, row, ' ', nil, style)
			col += 1
		}
		col = 0
		row += 1
	}
}

func handleUI(controller *DDPController, numPixels int) {
	defStyle := tcell.StyleDefault.Background(tcell.ColorDefault).Foreground(tcell.ColorDefault)
	textStyle := tcell.StyleDefault.Foreground(tcell.ColorGreen).Background(tcell.ColorDefault)
	boldTextStyle := textStyle.Bold(true)

	s, err := tcell.NewScreen()
	if err != nil {
		log.Fatalf("tcell NewScreen failed: %v", err)
	}
	if err := s.Init(); err != nil {
		log.Fatalf("tcell init failed: %v", err)
	}
	s.SetStyle(defStyle)
	s.Clear()

	quit := func() {
		// You have to catch panics in a defer, clean up, and re-raise
		// them - otherwise your application can die without leaving any
		// diagnostic trace.
		maybePanic := recover()
		s.Fini()
		if maybePanic != nil {
			panic(maybePanic)
		}
	}
	defer quit()

	debugMessage := func(msg string, args ...any) {
		msg = fmt.Sprintf(msg, args...)
		if len(msg) < 160 {
			msg = fmt.Sprintf("%-160s", msg)
		}

		drawText(s, 0, 20, 79, 21, textStyle, msg)
	}

	drawLightNum := func(num int) {
		drawText(s, 7, 0, 12, 0, boldTextStyle, strconv.Itoa(num))
	}
	drawSkip := func(skip int) {
		drawText(s, 7, 1, 12, 1, boldTextStyle, strconv.Itoa(skip))
	}

	drawText(s, 0, 0, 6, 0, textStyle, "Light#")
	drawText(s, 0, 1, 6, 1, textStyle, "  Skip")

	num := 0
	skip := 1

	drawText(s, 0, 10, 79, 10, textStyle, fmt.Sprintf("Num Pixels: %d", numPixels))

	for {
		s.Show()
		ev := s.PollEvent()
		debugMessage("got event: %+v", ev)

		next := num

		switch ev := ev.(type) {
		case *tcell.EventKey:
			switch ev.Key() {
			case tcell.KeyEscape:
				fallthrough
			case tcell.KeyCtrlC:
				return
			case tcell.KeyLeft:
				next = max(0, num-skip)
			case tcell.KeyRight:
				next = min(numPixels, num+skip)
			case tcell.KeyRune:
				switch ev.Rune() {
				case 'q':
					return
				case '+':
					skip *= 10
				case '-':
					skip = max(skip/10, 1)
				}
			}
		}

		if next != num {
			controller.SetOnPixel(next)
			num = next
		}

		drawLightNum(num)
		drawSkip(skip)
	}
}

type DDPController struct {
	conn                *ddp.DDPConn
	addr                *net.UDPAddr
	numPixels           int
	pixelChan, quitChan chan int
}

func newDDPController(conn *ddp.DDPConn, addr *net.UDPAddr, numPixels int) *DDPController {
	return &DDPController{
		conn:      conn,
		addr:      addr,
		numPixels: numPixels,
		pixelChan: make(chan int, 5),
		quitChan:  make(chan int, 1),
	}
}

func (c *DDPController) Start() {
	go c.loop()
}

func (c *DDPController) Stop() {
	c.quitChan <- 1
}

func (c *DDPController) SetOnPixel(pixelNum int) {
	c.pixelChan <- pixelNum
}

func (c *DDPController) loop() {
	ddpChans := make([]byte, c.numPixels*3)
	ticker := time.Tick(50 * time.Millisecond)
	last := -1

	for {
		select {
		case <-c.quitChan:
			return
		case onPixel := <-c.pixelChan:
			for i := 0; i < 3; i++ {
				if last >= 0 {
					ddpChans[last*3+i] = 0x00
				}
				ddpChans[onPixel*3+i] = 0xff
			}
			last = onPixel

			c.conn.SetPixels(ddpChans, c.addr)
		case <-ticker:
			c.conn.SetPixels(ddpChans, c.addr)
		}
	}
}

func main() {
	flag.Parse()

	if *numPixels == 0 {
		log.Fatal("--num_pixels is required")
	}
	if *controllerAddress == "" {
		log.Fatal("--controller is required")
	}

	addr, err := net.ResolveUDPAddr("udp", ddp.MaybeAddDDPPort(*controllerAddress))
	if err != nil {
		log.Fatalf("failed to resolve controller: %v", err)
	}

	conn, err := ddp.NewDDPConn(*verboseDDPConn)
	if err != nil {
		log.Fatalf("failed to make DDP connection: %v", err)
	}

	controller := newDDPController(conn, addr, *numPixels)
	defer controller.Stop()
	controller.Start()

	handleUI(controller, *numPixels)
}
