package main

import (
	"fmt"
	"sync"
)

var a string

func main() {
	var l sync.Mutex
	l.Lock()
	go func() {
		a = "hello, world"
		l.Unlock()
	}()
	l.Lock()
	fmt.Println(a)
}
