package de.fuchslabs.display

import java.security.MessageDigest
import java.security.SecureRandom
import java.security.cert.CertificateException
import java.security.cert.X509Certificate
import javax.net.ssl.HostnameVerifier
import javax.net.ssl.SSLContext
import javax.net.ssl.SSLSocketFactory
import javax.net.ssl.X509TrustManager

/**
 * Vertraut ausschliesslich einem Zertifikat, dessen SHA-256 (DER) dem erwarteten Fingerabdruck entspricht.
 * Ist [expected] null (manuelle Erstkopplung), wird der Fingerabdruck nur erfasst; der Aufrufer MUSS ihn
 * vom Nutzer bestaetigen lassen, bevor Geheimnisse (PIN) gesendet werden.
 */
class PinningTrustManager(private val expected: String?) : X509TrustManager {
    @Volatile var seenFingerprint: String? = null
        private set

    override fun checkClientTrusted(chain: Array<X509Certificate>, authType: String) =
        throw CertificateException("client auth not supported")

    override fun checkServerTrusted(chain: Array<X509Certificate>, authType: String) {
        if (chain.isEmpty()) throw CertificateException("empty chain")
        val fp = sha256Hex(chain[0].encoded)
        seenFingerprint = fp
        if (expected != null && !MessageDigest.isEqual(fp.toByteArray(), expected.toByteArray()))
            throw CertificateException("fingerprint mismatch")
    }

    override fun getAcceptedIssuers(): Array<X509Certificate> = emptyArray()

    fun socketFactory(): SSLSocketFactory =
        SSLContext.getInstance("TLS").apply { init(null, arrayOf(this@PinningTrustManager), SecureRandom()) }.socketFactory

    companion object {
        fun sha256Hex(der: ByteArray): String =
            MessageDigest.getInstance("SHA-256").digest(der).joinToString("") { "%02x".format(it) }

        /** Das Zertifikat ist selbstsigniert und an IPs ausgestellt; die Identitaet sichert der Fingerabdruck. */
        val AnyHost = HostnameVerifier { _, _ -> true }
    }
}
