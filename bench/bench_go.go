package main

import (
	"io"
	"log"
	"net"
	"strconv"
)

func main() {
	port := 8080
	l, err := net.Listen("tcp", ":"+strconv.Itoa(port))
	if err != nil {
		log.Println(err)
		return
	}
	defer l.Close()
	log.Println("listening on port", port)

	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println(err)
			continue
		}
		go handleConn(conn)
	}
}

func handleConn(conn net.Conn) {
	defer conn.Close()
	for {
		buffer := make([]byte, 1024)
		_, err := conn.Read(buffer)
		if err != nil && err != io.EOF {
			return
		}
		conn.Write([]byte("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\nHello, world!"))
	}
}
