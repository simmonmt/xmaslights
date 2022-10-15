package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"net"
)

const (
	ddp_idx_flags = 0
	ddp_idx_seq   = 1
	ddp_idx_type  = 2
	ddp_idx_id    = 3

	ddp_flags_ver1  = 0x40
	ddp_flags_query = 0x02
	ddp_flags_push  = 0x01
	ddp_id_status   = 251
)

type ddpPacket struct {
	hdr  [4]byte
	dOff uint32
	dLen uint16
	body []byte
}

func newDDPPacket(dataLen int) *ddpPacket {
	return &ddpPacket{
		hdr:  [4]byte{ddp_flags_ver1, 0, 0, 0},
		body: make([]byte, dataLen),
	}
}

func (p *ddpPacket) SetFlags(f byte) {
	p.hdr[ddp_idx_flags] = ddp_flags_ver1 | (f & 0x3f)
}

func (p *ddpPacket) SetSeqNo(s int) {
	p.hdr[ddp_idx_seq] = byte(s) & 0xf
}

func (p *ddpPacket) SetID(id byte) {
	p.hdr[ddp_idx_id] = id
}

func (p *ddpPacket) SetType(typ byte) {
	p.hdr[ddp_idx_type] = typ
}

func (p *ddpPacket) SetData(off int, d []byte) {
	if off < 0 || off > 0xffffffff {
		panic("bad off")
	}
	if len(d) > 0xffff {
		panic("long d")
	}

	p.dOff = uint32(off)
	p.dLen = uint16(len(d))
	p.body = d
}

func (p *ddpPacket) Write(w io.Writer) error {
	if _, err := w.Write(p.hdr[:]); err != nil {
		return err
	}
	if err := binary.Write(w, binary.BigEndian, p.dOff); err != nil {
		return err
	}
	if err := binary.Write(w, binary.BigEndian, p.dLen); err != nil {
		return err
	}
	if len(p.body) > 0 {
		if _, err := w.Write(p.body); err != nil {
			return err
		}
	}
	return nil
}

type packetWriter interface {
	WriteTo(p []byte, addr net.Addr) (n int, err error)
}

type DDPConn struct {
	conn              packetWriter
	seq               int
	maxChansPerPacket int
	verbose           bool
}

func NewDDPConn(verbose bool) (*DDPConn, error) {
	conn, err := net.ListenPacket("udp", ":0")
	if err != nil {
		return nil, err
	}

	return newDDPConn(conn, 1440, verbose), nil
}

func NewTestDDPConn(w packetWriter, maxChansPerPacket int) *DDPConn {
	return newDDPConn(w, maxChansPerPacket, true)
}

func newDDPConn(w packetWriter, maxChansPerPacket int, verbose bool) *DDPConn {
	return &DDPConn{
		conn:              w,
		seq:               1,
		maxChansPerPacket: maxChansPerPacket,
		verbose:           verbose,
	}
}

func (c *DDPConn) sendPacket(p *ddpPacket, dst *net.UDPAddr) error {
	if c.verbose {
		fmt.Printf("sending packet %+v\n", p)
	}

	var b bytes.Buffer
	if err := p.Write(&b); err != nil {
		return err
	}

	_, err := c.conn.WriteTo(b.Bytes(), dst)
	return err
}

func (c *DDPConn) makeDataFrame(off int, chans []byte, push bool) *ddpPacket {
	p := newDDPPacket(len(chans))
	if push {
		p.SetFlags(ddp_flags_push)
	}
	p.SetSeqNo(c.getSeq())
	p.SetType(1)
	p.SetID(1)
	p.SetData(off, chans)

	return p
}

func (c *DDPConn) getSeq() int {
	seq := c.seq
	c.seq++
	if c.seq > 15 {
		c.seq = 1
	}
	return seq
}

func (c *DDPConn) SetPixels(chans []byte, dst *net.UDPAddr) error {
	for off := 0; ; off += c.maxChansPerPacket {
		last := true
		chunk := chans
		if len(chans) > c.maxChansPerPacket {
			last = false
			chunk = chans[0:c.maxChansPerPacket]
		}

		p := c.makeDataFrame(int(off), chunk, last)
		if err := c.sendPacket(p, dst); err != nil {
			return err
		}

		if last {
			break
		}
		chans = chans[c.maxChansPerPacket:]
	}

	return nil
}
