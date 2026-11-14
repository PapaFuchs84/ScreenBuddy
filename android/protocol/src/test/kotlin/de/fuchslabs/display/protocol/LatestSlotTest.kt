package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicIntegerArray

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

    /**
     * Producer and consumer on different threads, coupled only by the scheduling contract: a delivery is
     * scheduled exactly when [LatestSlot.offer] returns true, and each delivery calls take() once. Tracks
     * the disposition of every single value instead of aggregate counts.
     */
    @Test fun everyValueIsDeliveredOrDroppedExactlyOnceAndTheLastIsDelivered() {
        repeat(20) { round ->
            val slot = LatestSlot<Int>()
            val total = 20_000
            val delivered = AtomicIntegerArray(total)
            val dropped = AtomicIntegerArray(total)
            val scheduled = AtomicInteger()
            val emptyDeliveries = AtomicInteger()
            val order = ArrayList<Int>() // consumer thread only; read after termination
            val consumer = Executors.newSingleThreadExecutor()
            val deliver = Runnable {
                val v = slot.take()
                if (v == null) emptyDeliveries.incrementAndGet() else { delivered.incrementAndGet(v); order += v }
            }
            val producer = Thread {
                for (i in 0 until total) {
                    if (slot.offer(i) { dropped.incrementAndGet(it) }) { scheduled.incrementAndGet(); consumer.execute(deliver) }
                    if (i % 97 == round) Thread.yield()
                }
            }
            producer.start(); producer.join()
            consumer.shutdown()
            assertTrue(consumer.awaitTermination(10, TimeUnit.SECONDS))

            for (v in 0 until total) {
                val d = delivered.get(v); val x = dropped.get(v)
                assertTrue("value $v: delivered=$d dropped=$x", (d == 1 && x == 0) || (d == 0 && x == 1))
            }
            assertEquals("final offered value must be delivered", 1, delivered.get(total - 1))
            assertEquals("no scheduled delivery may find the slot empty", 0, emptyDeliveries.get())
            assertEquals("one delivery per scheduling", scheduled.get(), order.size)
            assertEquals("deliveries keep the offer order", order.sorted(), order)
            assertNull(slot.take())
        }
    }
}
