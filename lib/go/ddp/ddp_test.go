package ddp

import (
	"bytes"
	"fmt"
	"net"
	"net/netip"
	"reflect"
	"strings"
	"testing"
)

type packet struct {
	data []byte
	addr net.Addr
}

func newPacket(data []byte, addr net.Addr) *packet {
	dCopy := make([]byte, len(data))
	copy(dCopy, data)

	return &packet{
		data: dCopy,
		addr: addr,
	}
}

func (p *packet) String() string {
	out := []string{}
	for _, d := range p.data {
		out = append(out, fmt.Sprintf("%02x", d))
	}
	return p.addr.String() + ":" + strings.Join(out, " ")
}

type packetSink struct {
	pkts []*packet
}

func newPacketSink() *packetSink {
	return &packetSink{
		pkts: []*packet{},
	}
}

func (s *packetSink) WriteTo(b []byte, addr net.Addr) (n int, err error) {
	s.pkts = append(s.pkts, newPacket(b, addr))
	return len(b), nil
}

func (s *packetSink) String() string {
	out := []string{}
	for _, p := range s.pkts {
		out = append(out, "["+p.String()+"]")
	}
	return strings.Join(out, ",")
}

func (s *packetSink) Packets() []*packet {
	return s.pkts
}

func TestDDPPacketWrite_QueryNoData(t *testing.T) {
	p := newDDPPacket(0)
	p.SetFlags(ddp_flags_query)
	p.SetID(ddp_id_status)
	p.dOff = 0x01020304
	p.dLen = 0x0506

	want := []byte{ddp_flags_ver1 | ddp_flags_query, 0, 0, ddp_id_status,
		0x1, 0x2, 0x3, 0x4,
		0x5, 0x6,
	}

	var b bytes.Buffer
	if err := p.Write(&b); err != nil || !reflect.DeepEqual(b.Bytes(), want) {
		t.Errorf("p.Write(_) = %v, %v, want %v, nil", b.Bytes(), err, want)
		return
	}
}

func TestSetPixels_Some(t *testing.T) {
	sink := newPacketSink()
	conn := NewTestDDPConn(sink, 5)
	addr := net.UDPAddrFromAddrPort(netip.MustParseAddrPort("127.0.0.1:3454"))

	pixels := []byte{0x11, 0x22, 0x33}

	if err := conn.SetPixels(pixels, addr); err != nil {
		t.Errorf("SetPixels = %v, want nil", err)
		return
	}

	wantPackets := []*packet{
		&packet{
			data: []byte{
				0x41, 0x01, 0x1, 0x1, // header (w/push)
				0x0, 0x0, 0x0, 0x0, // off
				0x0, 0x3, // len
				0x11, 0x22, 0x33, // data
			},
			addr: addr,
		},
	}

	if !reflect.DeepEqual(wantPackets, sink.Packets()) {
		t.Errorf("Bad packets: want %v, got %v", wantPackets, sink)
	}
}

func TestSetPixels_MultiPacket(t *testing.T) {
	sink := newPacketSink()
	conn := NewTestDDPConn(sink, 5)
	addr := net.UDPAddrFromAddrPort(netip.MustParseAddrPort("127.0.0.1:3454"))

	pixels := []byte{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}

	if err := conn.SetPixels(pixels, addr); err != nil {
		t.Errorf("SetPixels = %v, want nil", err)
		return
	}

	wantSink := newPacketSink()
	wantSink.pkts = []*packet{
		&packet{
			data: []byte{
				0x40, 0x01, 0x1, 0x1, // header
				0x0, 0x0, 0x0, 0x0, // off
				0x0, 0x5, // len
				0x11, 0x22, 0x33, 0x44, 0x55, // data
			},
			addr: addr,
		},
		&packet{
			data: []byte{
				0x41, 0x02, 0x1, 0x1, // header (w/push)
				0x0, 0x0, 0x0, 0x5, // off
				0x0, 0x3, // len
				0x66, 0x77, 0x88, // data
			},
			addr: addr,
		},
	}

	if !reflect.DeepEqual(wantSink, sink) {
		t.Errorf("Bad packets: want %v, got %v", wantSink, sink)
	}
}
