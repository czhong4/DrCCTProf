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

timeout
testing: warning: no tests to run
PASS
goleak: Errors on successful test run: found unexpected goroutines:
[Goroutine 18 in state chan send, with _/home/chongxin/DrCCTProf/testchannel.TestMain.func1 on top of the stack:
goroutine 18 [chan send]:
_/home/chongxin/DrCCTProf/testchannel.TestMain.func1(0xc0001140c0, 0x2)
        /home/chongxin/DrCCTProf/testchannel/channel1_test.go:16 +0x50
created by _/home/chongxin/DrCCTProf/testchannel.TestMain
        /home/chongxin/DrCCTProf/testchannel/channel1_test.go:14 +0x96
]
exit status 1
FAIL    _/home/chongxin/DrCCTProf/testchannel   1.942s
