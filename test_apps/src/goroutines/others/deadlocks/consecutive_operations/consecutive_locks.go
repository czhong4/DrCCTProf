package main

import (
	"fmt"
	"sync"
	"time"
)

var a string

func main() {
	var l sync.Mutex
	go func() {
		l.Lock()
		l.Lock()
		a = "hello, world"
	}()
	time.Sleep(time.Second)
	fmt.Println(a)
}
