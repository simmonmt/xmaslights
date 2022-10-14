package main

import (
	"fmt"
	"net"
)

type ddpPacket struct {
	hdr  [4]byte
	dOff uint32
	dLen uint16
	body []byte
}

func newDDPPacket(dataLen int) *ddpPacket {
}

type DDPConn struct {
	conn net.PacketConn
}

func NewDDPConn() (*DDPConn, error) {
	conn, err := net.ListenPacket("udp", ":0")
	if err != nil {
		return nil, err
	}

	return &DDPConn{conn: conn}, nil
}

func (c *DDPConn) sendPacket(d []byte, dst *net.UDPAddr) error {
	_, err := c.conn.WriteTo(d, dst)
	return err
}

func (c *DDPConn) SetPixels(pixels []byte, dst *net.UDPAddr) error {

	return fmt.Errorf("unimplemented")
}
