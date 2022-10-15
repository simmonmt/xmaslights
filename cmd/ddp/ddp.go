package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"net"
	"os"
	"path"

	"github.com/google/subcommands"
)

var (
	UsageError = errors.New("usage error")
)

type topFlags struct {
	Controller string
	NumPixels  int
	Verbose    bool
}

type topState struct {
	Conn      *DDPConn
	Addr      *net.UDPAddr
	NumPixels int
	Verbose   bool
}

func newUsageError(msg string) error {
	return fmt.Errorf("%w: %v", UsageError, msg)
}

func newTopStateFromTopFlags(ctx context.Context, tf *topFlags) (*topState, error) {
	if tf.NumPixels == 0 {
		return nil, newUsageError("--num_pixels is required")
	}
	if tf.Controller == "" {
		return nil, newUsageError("--controller is required")
	}

	addr, err := net.ResolveUDPAddr("udp", tf.Controller)
	if err != nil {
		return nil, fmt.Errorf("failed to resolve controller: %w", err)
	}

	conn, err := NewDDPConn(tf.Verbose)
	if err != nil {
		return nil, fmt.Errorf("failed to make DDP connection: %w", err)
	}

	return &topState{
		Conn:      conn,
		Addr:      addr,
		NumPixels: tf.NumPixels,
		Verbose:   tf.Verbose,
	}, nil
}

func handleCommandError(err error) subcommands.ExitStatus {
	fmt.Fprintln(os.Stderr, err)
	if errors.Is(err, UsageError) {
		return subcommands.ExitUsageError
	}
	return subcommands.ExitFailure
}

func main() {
	topFlags := &topFlags{}

	topFlagSet := flag.NewFlagSet("", flag.ExitOnError)
	topFlagSet.StringVar(&topFlags.Controller, "controller", "", "IP Address for the controller")
	topFlagSet.IntVar(&topFlags.NumPixels, "num_pixels", 0, "Number of controlled pixels")
	topFlagSet.BoolVar(&topFlags.Verbose, "verbose", false, "Additional output")

	topFlagSet.Parse(os.Args[1:])

	commander := subcommands.NewCommander(topFlagSet, path.Base(os.Args[0]))

	commander.Register(commander.HelpCommand(), "")
	commander.Register(commander.FlagsCommand(), "")
	commander.Register(commander.CommandsCommand(), "")

	commander.Register(&onCommand{}, "DDP operations")
	commander.Register(&offCommand{}, "DDP operations")
	commander.Register(&flashCommand{}, "DDP operations")

	ctx := context.Background()
	os.Exit(int(commander.Execute(ctx, topFlags)))
}
