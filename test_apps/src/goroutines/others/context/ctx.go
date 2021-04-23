package main

import (
	"context"
	//"fmt"
	"time"
)

func main() {
	// go func() {
	// 	_, cancel1 := context.WithCancel(context.Background())
	// 	cancel1()
	// }()
	// go func() {
	// 	_, cancel2 := context.WithTimeout(context.Background(), time.Second)
	// 	cancel2()
	// }()
	// go func() {
	// 	_, cancel3 := context.WithDeadline(context.Background(), time.Now().Add(time.Second))
	// 	cancel3()
	// }()
	// time.Sleep(2 * time.Second)

	// context1, cancel1 := context.WithCancel(context.Background())
	// context2, cancel2 := context.WithTimeout(context1, 2 * time.Second)
	// context2, cancel3 := context.WithDeadline(context2, time.Now().Add(time.Second))
	// cancel1()
	// cancel2()
	// cancel3()


	ctx := context.Background()
	context1, cancel1 := context.WithTimeout(ctx, time.Second)
	context2, cancel2 := context.WithTimeout(context1, 3 * time.Second)
	context3, cancel3 := context.WithCancel(context2)
	_, cancel4 := context.WithCancel(context3)

	a := 0
	if a > 0 {
		cancel1()
		cancel2()
		cancel3()
		cancel4()
	}
	time.Sleep(2 * time.Second)

	// context1, cancel1 := context.WithCancel(context.Background())
	// context2, cancel2 := context.WithCancel(context1)
	// _, cancel3 := context.WithCancel(context2)

	// cancel1()

	// a := 0
	// if a > 0 {
	// 	cancel2()
	// 	cancel3()
	// }
}