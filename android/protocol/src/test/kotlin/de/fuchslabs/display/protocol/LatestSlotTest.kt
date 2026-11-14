package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicInteger

class LatestSlotTest {
    @Test fun firstOfferSchedulesAndTakeEmpties() {
        val slot = LatestSlot<String>()
        assertNull(slot.take())
        assertTrue(slot.offer("a"))
        assertEquals("a", slot.take())
        assertNull(slot.take())
    }

    @Test fun newerValueSupersedesWithoutSecondDelivery() {
        val slot = LatestSlot<String>()
        val dropped = ArrayList<String>()
        assertTrue(slot.offer("a") { dropped += it })
        assertFalse(slot.offer("b") { dropped += it })
        assertFalse(slot.offer("c") { dropped += it })
        assertEquals(listOf("a", "b"), dropped)
        assertEquals("c", slot.take())
        assertNull(slot.take())
    }

    @Test fun offerAfterTakeSchedulesAgain() {
        val slot = LatestSlot<Int>()
        assertTrue(slot.offer(1))
        assertEquals(1, slot.take())
        assertTrue(slot.offer(2))
        assertEquals(2, slot.take())
    }

    @Test fun clearDropsPendingAndAllowsNewDelivery() {
        val slot = LatestSlot<String>()
        val dropped = ArrayList<String>()
        slot.clear { dropped += it } // empty: nothing to drop
        slot.offer("a")
        slot.clear { dropped += it }
        assertEquals(listOf("a"), dropped)
        assertNull(slot.take())
        assertTrue(slot.offer("b"))
    }

    @Test fun congestedConsumerNeverSeesMoreThanOnePendingValue() {
        val slot = LatestSlot<Int>()
        var scheduled = 0
        var droppedCount = 0
        for (i in 1..600) if (slot.offer(i) { droppedCount++ }) scheduled++
        assertEquals(1, scheduled)
        assertEquals(599, droppedCount)
        assertEquals(600, slot.take())
    }

    @Test fun everyValueIsDeliveredOrDroppedExactlyOnceAcrossThreads() {
        val slot = LatestSlot<Int>()
        val total = 20_000
        val dropped = AtomicInteger()
        val taken = AtomicInteger()
        val done = CountDownLatch(1)
        val producer = Thread {
            for (i in 1..total) slot.offer(i) { dropped.incrementAndGet() }
            done.countDown()
        }
        producer.start()
        while (done.count > 0L) if (slot.take() != null) taken.incrementAndGet()
        producer.join()
        if (slot.take() != null) taken.incrementAndGet()
        assertEquals(total, dropped.get() + taken.get())
    }
}
