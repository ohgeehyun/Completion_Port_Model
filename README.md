Completion_Port 모델을 사용한 asynchronous nonblocking socket

기존 Overlapped(callback)이 alertable wait상태에 들어간 스레드가 apc queue에서 작업을 해야하는 방식이었다면.
Completion_Port 모델은 apc queue 대신  Completion Port를 사용하여 중앙에서 관리하는 느낌의 apc큐를 사용한다.
이로인해 alertable wait상태의 문제점이었던 다른 스레드에게 일처리를 못시키는 것과 달리 workerthread를 만들어 다른 스레드에게 일을 처리시킬수있음.
멀티스레드환경에서 친화적이라는 것을 알 수 있다.

현재 mmorpg기준에서는 iocp를 사용하고 여기 cp가 Completion Port의 약자이다.

주요 function
//CreateIoCompletionPort //completionport를 만드는 것과 관찰하는것 2가지일이 가능한 함수
//GetQueuedCompletionStatus //결과를 관찰하는 함수

이 모델에서의 장단점은 적지않겠다.
장점은 위의 특성에서 설명한대로의 장점이지만 단점은 내가 아직 이걸 쓰면서 단점을 열거할 실력 및 지식이 없다.

