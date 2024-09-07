from i3ipc import Connection, Event, TickEvent
import time
import threading

class TestSendTick:
    def test_send_tick(self, server):
        class Reply:
            def __init__(self) -> None:
                self.is_shutdown = False

        print(f"Using server: {server}")
        
        reply = Reply()
        def wait_on_shutdown():
            def on_shutdown(i3, e):
                reply.is_shutdown = True

            conn1 = Connection(server)
            conn1.on(Event.SHUTDOWN, on_shutdown)
            try:
                conn1.main()
            except Exception as e:
                print(f"Encountered error: {e}")
        
        t1 = threading.Thread(target=wait_on_shutdown)
        t1.start()
        time.sleep(1)  # A small wait time to ensure that the subscribe goes through first

        try:
            conn2 = Connection(server)
            conn2.command("exit")
            t1.join()
        except Exception as e:
            print(f"Encountered error: {e}")
        
        assert reply.is_shutdown == True
