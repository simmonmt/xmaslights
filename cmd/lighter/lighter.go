package main

import (
	"fmt"
	"log"
	"strconv"

	"github.com/gdamore/tcell"
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
}

func main() {
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
		// You have to catch panics in a defer, clean up, and
		// re-raise them - otherwise your application can
		// die without leaving any diagnostic trace.
		maybePanic := recover()
		s.Fini()
		if maybePanic != nil {
			panic(maybePanic)
		}
	}
	defer quit()

	num := 0
	drawText(s, 0, 0, 6, 0, textStyle, "Light#")

	for {
		s.Show()
		ev := s.PollEvent()
		drawText(s, 0, 20, 79, 25, textStyle, fmt.Sprintf("got event: %+v", ev))

		switch ev := ev.(type) {
		case *tcell.EventKey:
			switch ev.Key() {
			case tcell.KeyEscape:
				fallthrough
			case tcell.KeyCtrlC:
				return
			case tcell.KeyLeft:
				num = max(0, num-1)
			case tcell.KeyRight:
				num += 1
			case tcell.KeyRune:
				if ev.Rune() == 'q' {
					return
				}
			}
		}

		drawText(s, 7, 0, 12, 0, boldTextStyle, strconv.Itoa(num))
	}
}
