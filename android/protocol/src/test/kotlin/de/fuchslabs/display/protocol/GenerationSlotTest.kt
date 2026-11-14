package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicIntegerArray

class GenerationSlotTest {
    @Test fun valueKeepsTheGenerationItWasOfferedWith() {
        val slot = GenerationSlot<String>()
        assertTrue(slot.offer(3, "a"))
        val e = slot.take()!!
        assertEquals(3, e.generation)
        assertEquals("a", e.value)
        assertNull(slot.take())
    }

    @Test fun newerValueOfSameGenerationSupersedesWithoutSecondDelivery() {
        val slot = GenerationSlot<String>()
        val dropped = ArrayList<String>()
        assertTrue(slot.offer(1, "a") { dropped += it })
        assertFalse(slot.offer(1, "b") { dropped += it })
        assertEquals(listOf("a"), dropped)
        assertEquals("b", slot.take()!!.value)
    }

    /** FrameView: a decode task scheduled in epoch E runs only after the target changed and frame B arrived. */
    @Test fun staleConsumerTakesNewFrameWithItsOwnEpoch() {
        val slot = GenerationSlot<String>()
        var epoch = 0
        assertTrue(slot.offer(epoch, "A")) // decode task 1 scheduled, not yet run
        epoch++; slot.advance(epoch)       // clearPending()
        assertTrue(slot.offer(epoch, "B")) // decode task 2 scheduled
        val first = slot.take()!!          // task 1 finally runs
        assertEquals("B", first.value)
        assertEquals(epoch, first.generation) // shown, not discarded as stale
        assertNull(slot.take())            // task 2 finds nothing: nothing was lost
    }

    @Test fun advanceDiscardsPendingOlderValue() {
        val slot = GenerationSlot<String>()
        val dropped = ArrayList<String>()
        slot.offer(1, "a")
        slot.advance(1) { dropped += it } // same generation: kept
        assertTrue(dropped.isEmpty())
        slot.advance(2) { dropped += it }
        assertEquals(listOf("a"), dropped)
        assertNull(slot.take())
        assertTrue(slot.offer(2, "b"))
    }

    /** DisplayClient: a callback of the previous connection resumes after the new connection queued a frame. */
    @Test fun olderGenerationNeverReplacesNewerPendingValue() {
        val slot = GenerationSlot<String>()
        val dropped = ArrayList<String>()
        assertTrue(slot.offer(2, "new") { dropped += it })
        assertFalse(slot.offer(1, "old") { dropped += it })
        assertEquals(listOf("old"), dropped)
        val e = slot.take()!!
        assertEquals(2, e.generation)
        assertEquals("new", e.value)
    }

    @Test fun olderGenerationIsRejectedAfterAdvanceEvenWhenSlotIsEmpty() {
        val slot = GenerationSlot<String>()
        val dropped = ArrayList<String>()
        slot.advance(5)
        assertFalse(slot.offer(4, "old") { dropped += it })
        assertEquals(listOf("old"), dropped)
        assertNull(slot.take())
        assertTrue(slot.offer(5, "cur"))
    }

    @Test fun olderGenerationIsRejectedAfterNewerOneWasDelivered() {
        val slot = GenerationSlot<String>()
        slot.offer(2, "new")
        slot.take()
        assertFalse(slot.offer(1, "old"))
        assertNull(slot.take())
    }

    @Test fun advanceNeverMovesBackwards() {
        val slot = GenerationSlot<String>()
        slot.advance(5)
        slot.advance(3)
        assertFalse(slot.offer(4, "old"))
        assertTrue(slot.offer(5, "cur"))
    }

    @Test fun clearDropsPendingAndAllowsNewDelivery() {
        val slot = GenerationSlot<String>()
        val dropped = ArrayList<String>()
        slot.clear { dropped += it }
        slot.offer(1, "a")
        slot.clear { dropped += it }
        assertEquals(listOf("a"), dropped)
        assertTrue(slot.offer(1, "b"))
    }

    /**
     * Two connections race: the old one keeps offering while the new one starts. Deliveries are driven by the
     * scheduling contract. Every value ends up delivered once or dropped once, no old value is delivered
     * after a new one, and the final value of the new generation is always delivered.
     */
    @Test fun racingGenerationsLoseNoCurrentValue() {
        repeat(20) {
            val slot = GenerationSlot<Int>()
            val perGen = 5_000
            val delivered = AtomicIntegerArray(2 * perGen)
            val dropped = AtomicIntegerArray(2 * perGen)
            val order = java.util.Collections.synchronizedList(ArrayList<Int>())
            val consumer = Executors.newSingleThreadExecutor()
            val deliver = Runnable {
                val e = slot.take() ?: return@Runnable
                delivered.incrementAndGet(e.value); order += e.generation
            }
            fun produce(gen: Int) = Thread {
                for (i in 0 until perGen) {
                    val v = (gen - 1) * perGen + i
                    if (slot.offer(gen, v) { dropped.incrementAndGet(it) }) consumer.execute(deliver)
                }
            }
            val old = produce(1); val new = produce(2)
            old.start(); new.start(); old.join(); new.join()
            consumer.shutdown()
            assertTrue(consumer.awaitTermination(10, TimeUnit.SECONDS))
            for (v in 0 until 2 * perGen) assertEquals("value $v", 1, delivered.get(v) + dropped.get(v))
            assertEquals("final value of the new generation", 1, delivered.get(2 * perGen - 1))
            assertEquals(order.sorted(), order.toList())
            assertNull(slot.take())
        }
    }
}
