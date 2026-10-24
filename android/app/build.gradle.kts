plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}
android {
    namespace = "de.fuchslabs.display"
    compileSdk = 34
    defaultConfig {
        applicationId = "de.fuchslabs.display"
        minSdk = 26
        targetSdk = 34
        versionCode = 2
        versionName = "0.2.0-preview"
    }
    buildTypes { release { isMinifyEnabled = false } }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
    buildFeatures { viewBinding = false }
}
dependencies {
    implementation(project(":protocol"))
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
    // ZXing-Scanner ohne Google Play Services (Kamera + ZXing-Decoder lokal).
    implementation("com.journeyapps:zxing-android-embedded:4.3.0")
}
