package main

import (
	"context"
	"flag"
	"fmt"
	"regexp"
	"strconv"
	"strings"

	"github.com/google/subcommands"
)

var (
	pixelRange = regexp.MustCompile(`^([0-9]+)\.\.([0-9]+)$`)
)

func parsePixelIDs(ids string) ([]int, error) {
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

type onCommand struct{}

func (c *onCommand) Name() string             { return "on" }
func (c *onCommand) Synopsis() string         { return "Turn on pixels" }
func (c *onCommand) Usage() string            { return `on n1..n2|n1,n2,...` }
func (c *onCommand) SetFlags(f *flag.FlagSet) {}

func (c *onCommand) Execute(ctx context.Context, f *flag.FlagSet, args ...interface{}) subcommands.ExitStatus {
	state, err := newTopStateFromTopFlags(ctx, args[0].(*topFlags))
	if err != nil {
		return handleCommandError(err)
	}

	if len(f.Args()) != 1 {
		return handleCommandError(newUsageError("missing pixels argument"))
	}
	toOn, err := parsePixelIDs(f.Args()[0])
	if err != nil {
		return handleCommandError(fmt.Errorf("invalid pixels argument: %v", err))
	}

	pixels := make([]byte, state.NumPixels)
	for _, p := range toOn {
		if p >= state.NumPixels {
			return handleCommandError(fmt.Errorf("pixel %v out of range", p))
		}
		pixels[p] = 0xff
	}

	if err := state.Conn.SetPixels(pixels, state.Addr); err != nil {
		return handleCommandError(fmt.Errorf("failed to set pixels: %v", err))
	}
	return subcommands.ExitSuccess
}

type offCommand struct{}

func (c *offCommand) Name() string             { return "on" }
func (c *offCommand) Synopsis() string         { return "Turn off pixels" }
func (c *offCommand) Usage() string            { return `off n1..n2|n1,n2,...` }
func (c *offCommand) SetFlags(f *flag.FlagSet) {}

func (c *offCommand) Execute(ctx context.Context, f *flag.FlagSet, args ...interface{}) subcommands.ExitStatus {
	return handleCommandError(fmt.Errorf("unimplemented"))
}

type flashCommand struct{}

func (c *flashCommand) Name() string             { return "flash" }
func (c *flashCommand) Synopsis() string         { return "Flash pixels" }
func (c *flashCommand) Usage() string            { return `flash n1..n2|n1,n2,...` }
func (c *flashCommand) SetFlags(f *flag.FlagSet) {}

func (c *flashCommand) Execute(ctx context.Context, f *flag.FlagSet, args ...interface{}) subcommands.ExitStatus {
	return handleCommandError(fmt.Errorf("unimplemented"))
}
