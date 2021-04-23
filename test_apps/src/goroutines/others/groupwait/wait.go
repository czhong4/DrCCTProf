package main

import (
	"sync"
	"time"
	"fmt"
	"unsafe"
)

type s struct {
	a int
	wg sync.WaitGroup
}

func main() {
	var group sync.WaitGroup
	var w s
	fmt.Println(unsafe.Sizeof(group))
	group.Add(10)
	w.wg.Add(10)
	go func() {
		for i := 0; i < 10; i++ {
			go func() {
				w.wg.Done()
			}()
		}
		w.wg.Wait()
		for i := 0; i < 10; i++ {
			go func() {
				group.Done()
			}()
			group.Wait()
		}
		fmt.Println("finished")
	}()
	time.Sleep(2 * time.Second)
}
