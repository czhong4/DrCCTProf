package main

import (
	"sync"
	"time"
)

type statusManager struct {
	podStatusesLock  sync.Mutex
	podStatusChannel chan bool
}

func (s *statusManager) Start() {
	go func() {
		for i := 0; i < 2; i++ {
			s.syncBatch()
		}
	}()
}

func (s *statusManager) syncBatch() {
	<-s.podStatusChannel
	s.DeletePodStatus()
}

func (s *statusManager) DeletePodStatus() {
	s.podStatusesLock.Lock()
	defer s.podStatusesLock.Unlock()
}

func (s *statusManager) SetPodStatus() {
	s.podStatusesLock.Lock()
	defer s.podStatusesLock.Unlock()
	s.podStatusChannel <- true
}

func NewStatusManager() *statusManager {
	return &statusManager{
		podStatusChannel: make(chan bool),
	}
}

/// G1 						G2							G3
/// s.Start()
/// s.syncBatch()
/// 						s.SetPodStatus()
/// <-s.podStatusChannel
/// 						s.podStatusesLock.Lock()
/// 						s.podStatusChannel <- true
/// 						s.podStatusesLock.Unlock()
/// 						return
/// s.DeletePodStatus()
/// 													s.podStatusesLock.Lock()
/// 													s.podStatusChannel <- true
/// s.podStatusesLock.Lock()
/// -----------------------------G1,G3 deadlock----------------------------
func main() {
	s := NewStatusManager()
	go s.Start()
	go s.SetPodStatus() // G2
	go s.SetPodStatus() // G3
	time.Sleep(3 * time.Second)
}
