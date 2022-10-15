package main

import (
	"context"
	"flag"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/google/subcommands"
)

var (
	pixelRange = regexp.MustCompile(`^([0-9]+)\.\.([0-9]+)$`)
)

func parsePixelIDs(ids string, numPixels int) ([]int, error) {
	if ids == "all" {
		out := make([]int, numPixels)
		for i := 0; i < numPixels; i++ {
			out[i] = i
		}
		return out, nil
	}

	parts := pixelRange.FindStringSubmatch(ids)
	if parts != nil {
		from, err := strconv.ParseUint(parts[1], 10, 32)
		if err != nil {
			return nil, fmt.Errorf("invalid from value %v", parts[1])
		}

		to, err := strconv.ParseUint(parts[2], 10, 32)
		if err != nil {
			return nil, fmt.Errorf("invalid to value %v", parts[2])
		}

		if to < from {
			return nil, fmt.Errorf("bad range; from >= to")
		}

		if from >= uint64(numPixels) || to >= uint64(numPixels) {
			return nil, fmt.Errorf("from/to not in range 0..%v", numPixels)
		}

		num := int(to - from + 1)
		out := make([]int, num)
		for i := 0; i < num; i++ {
			out[i] = int(from) + i
		}
		return out, nil
	}

	parts = strings.Split(ids, ",")
	out := make([]int, len(parts))
	for i, part := range parts {
		num, err := strconv.ParseUint(part, 10, 32)
		if err != nil {
			return nil, fmt.Errorf("bad pixel %s: %v", part, err)
		}
		out[i] = int(num)
	}

	return out, nil
}

type onCommand struct {
	color uint
}

func (c *onCommand) Name() string     { return "on" }
func (c *onCommand) Synopsis() string { return "Turn on pixels" }
func (c *onCommand) Usage() string    { return "Usage: on --color 0xrrggbb all|n1..n2|n1,n2,...\n" }
func (c *onCommand) SetFlags(f *flag.FlagSet) {
	f.UintVar(&c.color, "color", 0xffffff, "Color to use")
}

func (c *onCommand) Execute(ctx context.Context, f *flag.FlagSet, args ...interface{}) subcommands.ExitStatus {
	state, err := newTopStateFromTopFlags(ctx, args[0].(*topFlags))
	if err != nil {
		return handleCommandError(err)
	}

	if len(f.Args()) != 1 {
		return handleCommandError(newUsageError("missing pixels argument"))
	}
	toOn, err := parsePixelIDs(f.Args()[0], state.NumPixels)
	if err != nil {
		return handleCommandError(fmt.Errorf("invalid pixels argument: %v", err))
	}

	if c.color > 0xffffff {
		return handleCommandError(newUsageError("invalid color value; must be 0xrrggbb"))
	}

	chans := make([]byte, state.NumPixels*3)
	for _, p := range toOn {
		chans[p*3] = byte(c.color >> 16)
		chans[p*3+1] = byte((c.color >> 8) & 0xff)
		chans[p*3+2] = byte(c.color & 0xff)
	}

	if err := state.Conn.SetPixels(chans, state.Addr); err != nil {
		return handleCommandError(fmt.Errorf("failed to set pixels: %v", err))
	}
	return subcommands.ExitSuccess
}

type offCommand struct{}

func (c *offCommand) Name() string             { return "off" }
func (c *offCommand) Synopsis() string         { return "Turn off pixels" }
func (c *offCommand) Usage() string            { return "Usage: off\n" }
func (c *offCommand) SetFlags(f *flag.FlagSet) {}

func (c *offCommand) Execute(ctx context.Context, f *flag.FlagSet, args ...interface{}) subcommands.ExitStatus {
	state, err := newTopStateFromTopFlags(ctx, args[0].(*topFlags))
	if err != nil {
		return handleCommandError(err)
	}

	if len(f.Args()) != 0 {
		return handleCommandError(newUsageError("extra arguments"))
	}

	chans := make([]byte, state.NumPixels*3)
	if err := state.Conn.SetPixels(chans, state.Addr); err != nil {
		return handleCommandError(fmt.Errorf("failed to set pixels: %v", err))
	}
	return subcommands.ExitSuccess
}

type flashCommand struct {
	color          uint
	period, length time.Duration
}

func (c *flashCommand) Name() string     { return "flash" }
func (c *flashCommand) Synopsis() string { return "Flash pixels" }
func (c *flashCommand) Usage() string    { return `flash n1..n2|n1,n2,...` }
func (c *flashCommand) SetFlags(f *flag.FlagSet) {
	f.UintVar(&c.color, "color", 0xffffff, "Color to use")
	f.DurationVar(&c.period, "period", time.Duration(500)*time.Millisecond, "Flash period")
	f.DurationVar(&c.length, "length", time.Duration(5)*time.Second, "How long to flash")
}

func (c *flashCommand) Execute(ctx context.Context, f *flag.FlagSet, args ...interface{}) subcommands.ExitStatus {
	state, err := newTopStateFromTopFlags(ctx, args[0].(*topFlags))
	if err != nil {
		return handleCommandError(err)
	}

	if len(f.Args()) != 1 {
		return handleCommandError(newUsageError("missing pixels argument"))
	}
	toOn, err := parsePixelIDs(f.Args()[0], state.NumPixels)
	if err != nil {
		return handleCommandError(fmt.Errorf("invalid pixels argument: %v", err))
	}

	if c.color > 0xffffff {
		return handleCommandError(newUsageError("invalid color value; must be 0xrrggbb"))
	}

	onChans := make([]byte, state.NumPixels*3)
	for _, p := range toOn {
		onChans[p*3] = byte(c.color >> 16)
		onChans[p*3+1] = byte((c.color >> 8) & 0xff)
		onChans[p*3+2] = byte(c.color & 0xff)
	}

	offChans := make([]byte, state.NumPixels*3)

	until := time.Now().Add(c.length)
	isOn := false
	for time.Now().Before(until) {
		chans := onChans
		if isOn {
			chans = offChans
		}

		if err := state.Conn.SetPixels(chans, state.Addr); err != nil {
			return handleCommandError(fmt.Errorf("failed to set pixels: %v", err))
		}
		isOn = !isOn

		sleep := c.period
		if left := until.Sub(time.Now()); left < sleep {
			sleep = left
		}
		time.Sleep(sleep)
	}

	return subcommands.ExitSuccess
}
