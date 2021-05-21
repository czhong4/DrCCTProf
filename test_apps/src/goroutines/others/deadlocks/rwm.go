package main

import (
	"fmt"
	"sync"
	"time"
)

func main() {
	var rwm sync.RWMutex
	var rwm1 sync.RWMutex
	var m sync.Mutex
	m.Lock()
	go func() {
		rwm.RLock()
		rwm1.RLock()
		rwm.RLock()
		rwm1.RLock()
		fmt.Println("reader")
		rwm1.RUnlock()
		rwm.RUnlock()
		rwm1.RUnlock()
		rwm.RUnlock()
	}()
	go func() {
		time.Sleep(time.Second)
		rwm.Lock()
		fmt.Println("writer")
		rwm.Unlock()
	}()
	m.Unlock()
	time.Sleep(2 * time.Second)
}
