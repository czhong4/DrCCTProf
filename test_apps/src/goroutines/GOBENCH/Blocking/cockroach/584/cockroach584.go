package main

import (
	"sync"
	"time"
)

type Gossip struct {
	mu     sync.Mutex
	closed bool
}

func lock_wrapper(mu *sync.Mutex) {
	mu.Lock()
}

func (g *Gossip) bootstrap() {
	for {
		g.mu.Lock()
		if g.closed {
			/// Missing g.mu.Unlock
			break
		}
		g.mu.Unlock()
		break
	}
}

func (g *Gossip) manage() {
	for {
		lock_wrapper(&g.mu)
		if g.closed {
			/// Missing g.mu.Unlock
			break
		}
		g.mu.Unlock()
		break
	}
}
func main() {
	g := &Gossip{
		closed: true,
	}
	go func() {
		g.bootstrap()
		g.manage()
	}()
	time.Sleep(2 * time.Second)
}
