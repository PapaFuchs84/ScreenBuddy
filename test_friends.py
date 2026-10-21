# © 2026 FuchsLabs
"""Tests für das Zusammenführen der Freundeslisten (ohne Netzwerk). Aufruf: python test_friends.py"""
from friends import Account, merge_accounts, suggest_links


def names(people):
    return [p.name for p in people]


def test_gleichzeitig_online_nur_einmal():
    accs = [Account("discord", "1", "Basti", "online", "Spielt Battlefield 6"),
            Account("steam", "7656", "Basti", "online", "Spielt Battlefield 6"),
            Account("xbox", "xu1", "Basti", "online")]
    people = merge_accounts(accs, [], True)
    assert len(people) == 1, people
    assert people[0].platforms == ("discord", "steam", "xbox")
    assert people[0].status == "online" and people[0].activity == "Spielt Battlefield 6"


def test_schreibweise_wird_ignoriert():
    accs = [Account("discord", "1", "Basti_84", "online"), Account("steam", "2", "basti-84", "online"),
            Account("xbox", "3", "BÀSTI 84", "offline")]
    assert len(merge_accounts(accs, [], True)) == 1


def test_online_hat_vorrang_und_spiel_kommt_vom_spielenden_konto():
    accs = [Account("discord", "1", "Lena", "idle"), Account("xbox", "2", "Lena", "online", "Spielt Halo"),
            Account("steam", "3", "Lena", "offline")]
    p, = merge_accounts(accs, [], True)
    assert p.status == "online" and p.activity == "Spielt Halo" and len(p.platforms) == 3


def test_verknuepfung_bei_verschiedenen_namen():
    accs = [Account("discord", "1", "Kevin", "online"), Account("steam", "2", "xX_K3v_Xx", "online"),
            Account("xbox", "3", "KevinGamer", "online")]
    people = merge_accounts(accs, ["Kevin|discord:1|steam:2|xbox:KevinGamer"], True)
    assert len(people) == 1 and people[0].name == "Kevin" and len(people[0].platforms) == 3


def test_zwei_personen_gleichen_namens_auf_derselben_plattform_bleiben_getrennt():
    accs = [Account("steam", "1", "Alex", "online"), Account("steam", "2", "Alex", "online"),
            Account("discord", "3", "Alex", "online")]
    people = merge_accounts(accs, [], True)
    assert len(people) == 2, people            # nie zwei Steam-Konten in einer Person
    assert sum(len(p.platforms) for p in people) == 3


def test_ohne_automatik_bleiben_gleiche_namen_getrennt():
    accs = [Account("discord", "1", "Tom", "online"), Account("steam", "2", "Tom", "online")]
    assert len(merge_accounts(accs, [], False)) == 2
    assert len(merge_accounts(accs, ["Tom|discord:1|steam:2"], False)) == 1


def test_jedes_konto_nur_einmal_auch_bei_ueberlappenden_verknuepfungen():
    accs = [Account("discord", "1", "A", "online"), Account("steam", "2", "B", "online"), Account("xbox", "3", "C", "online")]
    people = merge_accounts(accs, ["Eins|discord:1|steam:2", "Zwei|steam:2|xbox:3"], True)
    assert sum(len(p.platforms) for p in people) == 3     # kein Konto doppelt gezählt
    assert len(people) == 2


def test_vorschlaege():
    accs = [{"platform": "discord", "id": "1", "name": "Bastian"}, {"platform": "steam", "id": "2", "name": "Basti"},
            {"platform": "xbox", "id": "3", "name": "Tom"}, {"platform": "steam", "id": "4", "name": "Tim"}]
    s = suggest_links(accs, [], True)
    assert len(s) == 1 and {s[0]["a"]["name"], s[0]["b"]["name"]} == {"Bastian", "Basti"}, s
    assert suggest_links(accs, ["Basti|discord:1|steam:2"], True) == []        # schon verknüpft
    same = [{"platform": "discord", "id": "1", "name": "Kim"}, {"platform": "steam", "id": "2", "name": "kim"}]
    assert suggest_links(same, [], True) == []                                  # macht die Automatik
    assert len(suggest_links(same, [], False)) == 1


if __name__ == "__main__":
    tests = [v for k, v in sorted(globals().items()) if k.startswith("test_")]
    for t in tests:
        t()
        print("ok  ", t.__name__)
    print(f"{len(tests)} Tests bestanden")
