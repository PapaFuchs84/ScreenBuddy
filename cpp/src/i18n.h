// © 2026 FuchsLabs
// Sprache der App: Deutsch (Quelltexte) oder Englisch. tr() übersetzt deutsche Texte beim Zeichnen bzw. vor der
// Ausgabe an die Einstellungsseite: erst als ganzer Satz, sonst über Muster ("Spielt X" → "Playing X").
// Unbekannte Texte (Namen, Blog-Titel) bleiben unverändert.
#pragma once

#include <string>

namespace fl::i18n {

void set_lang(const std::string& lang);
const std::string& lang();
std::string tr(const std::string& text);

}  // namespace fl::i18n

namespace fl {
using i18n::tr;
}
