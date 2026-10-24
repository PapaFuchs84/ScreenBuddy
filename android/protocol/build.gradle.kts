// Reines JVM-Modul: Protokoll-Parsing ohne Android-Abhaengigkeit (JVM-Unit-Tests ohne SDK).
plugins { id("org.jetbrains.kotlin.jvm") }
dependencies {
    // Auf Android liefert die Plattform org.json; daher nur compileOnly.
    compileOnly("org.json:json:20240303")
    testImplementation("org.json:json:20240303")
    testImplementation("junit:junit:4.13.2")
}
