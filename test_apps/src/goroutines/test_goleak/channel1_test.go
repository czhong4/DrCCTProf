package m

import (
	"time"
	"fmt"
	"testing"
	"go.uber.org/goleak"
)

func TestMain(m *testing.M) {
	ch := make(chan int)
	a := 2

	go func() {
		time.Sleep(time.Second)
		ch <- a
		fmt.Println("finished")
	}()
	
	select {
		case res := <- ch:
			fmt.Println(res)
		case <- time.After(500 * time.Millisecond):
			fmt.Println("timeout")
	}
	time.Sleep(time.Second)
	defer goleak.VerifyTestMain(m)
}
