package main

import (
	"sync"
	"time"
	"fmt"
)

func main() {
	ch := make(chan int)
	var m sync.Mutex
	var a int

	go func() {
		//time.Sleep(time.Second)
		m.Lock()
		m.Unlock()
		a = <- ch
		fmt.Println("finish0")
	}()

	go func() {
		time.Sleep(time.Second)
		m.Lock()
		ch <- a
		m.Unlock()
		fmt.Println("finish1")
	}()

	time.Sleep(2*time.Second)
}

