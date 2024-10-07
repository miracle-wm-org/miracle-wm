from i3ipc import Connection, Event, TickEvent
import time
import threading

class TestSendTick:
    def test_send_tick(self, server):
        class Reply:
            def __init__(self) -> None:
                self.reply = ""

        reply = Reply()
        def wait_on_tick():
            def on_tick(i3, e: TickEvent):
                if not e.first:
                    reply.reply = e.payload
                    assert(e.payload == "ping")
                    conn1.main_quit()

            conn1 = Connection(server.ipc)
            conn1.on(Event.TICK, on_tick)
            conn1.main()
        
        t1 = threading.Thread(target=wait_on_tick)
        t1.start()
        time.sleep(1)  # A small wait time to ensure that the subscribe goes through first

        try:
            conn2 = Connection(server.ipc)
            conn2.send_tick("ping")
            t1.join()
        except Exception as e:
            print(f"Encountered error: {e}")
        
        assert reply.reply == "ping"
