package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test

class AutoOwnershipTest {
    @Test fun autoIntentAtManualForegroundDisplayKeepsManualOwnership() {
        assertFalse(AutoOwnership.afterNewIntent(auto = true, wasAuto = false, started = true))
    }

    @Test fun autoIntentBringingStoppedDisplayForwardTakesOwnership() {
        assertTrue(AutoOwnership.afterNewIntent(auto = true, wasAuto = false, started = false))
    }

    @Test fun repeatedAutoIntentKeepsAutoOwnership() {
        assertTrue(AutoOwnership.afterNewIntent(auto = true, wasAuto = true, started = true))
        assertTrue(AutoOwnership.afterNewIntent(auto = true, wasAuto = true, started = false))
    }

    @Test fun manualIntentAlwaysTakesOwnershipBack() {
        for (wasAuto in listOf(true, false)) for (started in listOf(true, false))
            assertFalse(AutoOwnership.afterNewIntent(auto = false, wasAuto = wasAuto, started = started))
    }
}
