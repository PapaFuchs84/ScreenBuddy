// Simulierte Uhr, keine Geräte, keine Ports, keine Grafik.
#include "plugin_schedule.h"
#include <cstdio>
#include <cmath>
using namespace fl::plugins::schedule;
int passed=0,failed=0;
#define CHECK(x) do { if(x) ++passed; else { ++failed; std::printf("FAIL %d: %s\n",__LINE__,#x); } } while(0)
int main() {
    ZoneInput z{"a",Mode::rotate,true,{{"one",3,true},{"two",5,true},{"three",8,true}}};
    std::map<std::string,ZoneState> s;
    Budget cut;
    for (int round=0;round<2;++round) {
        CHECK(advance({z},s,round*16,cut).zones[0].block=="one");
        CHECK(advance({z},s,round*16+3,cut).zones[0].block=="two");
        CHECK(advance({z},s,round*16+8,cut).zones[0].block=="three");
    }
    s.clear(); z.entries[1].available=false;
    advance({z},s,0,cut); CHECK(advance({z},s,3,cut).zones[0].block=="three");
    for(auto& e:z.entries)e.available=false;
    CHECK(!advance({z},s,4,cut).zones[0].block);
    z.entries[0].available=z.entries[1].available=true; z.mode=Mode::first_available;
    CHECK(advance({z},s,5,cut).zones[0].block=="one");
    CHECK(advance({z},s,100,cut).zones[0].block=="one");
    z.entries[0].available=false; CHECK(advance({z},s,101,cut).zones[0].block=="two");
    z.mode=Mode::rotate;z.entries[0].available=true;s.clear();
    auto fade=budget_for(OutputKind::monitor,{},0,10,160,480,0);
    advance({z},s,0,fade);
    auto r=advance({z},s,3,fade); CHECK(r.animating && r.zones[0].mix==0);
    auto twice=advance({z},s,3,fade); CHECK(twice.animating && twice.zones[0].mix==r.zones[0].mix);
    CHECK(advance({z},s,3.101,fade).zones[0].mix==.25f);
    CHECK(advance({z},s,3.201,fade).zones[0].mix==.5f);
    CHECK(advance({z},s,3.301,fade).zones[0].mix==.75f);
    CHECK(!advance({z},s,3.401,fade).animating);
    s.clear();advance({z},s,0,fade);advance({z},s,3,fade);
    CHECK(!advance({z},s,3.9,fade).animating);
    // Verspätung mitten im Monitorübergang bleibt ein Zwischenbild.
    s.clear();advance({z},s,0,fade);advance({z},s,3,fade);
    auto middle=advance({z},s,3.25,fade);CHECK(middle.animating && middle.zones[0].mix==.5f);
    auto phone=budget_for(OutputKind::companion,{},0,0,160,480,1);
    s.clear();advance({z},s,0,phone);advance({z},s,3,phone);
    CHECK(!advance({z},s,3.8,phone).animating);
    auto serial=budget_for(OutputKind::serial,1000000,0,10,160,480,0);
    CHECK(serial.step_seconds >= .05+.01+serial_bytes(160,480)/1000000.0/4);
    s.clear();advance({z},s,0,serial);advance({z},s,3,serial);
    CHECK(advance({z},s,3.01,serial).zones[0].mix==0);
    CHECK(s["a"].fade->start==3);
    CHECK(advance({z},s,3.25,serial).zones[0].mix==.25f);
    CHECK(advance({z},s,3.5,serial).zones[0].mix==.5f);
    // Datenverlust mitten in der Reihenfolge geht vorwärts, erst am Ende zurück.
    auto ordered=z;ordered.entries[2].available=true;
    s.clear();advance({ordered},s,0,cut);advance({ordered},s,3,cut);
    ordered.entries[1].available=false;
    CHECK(advance({ordered},s,4,cut).zones[0].block=="three");
    ordered.entries[2].available=false;
    CHECK(advance({ordered},s,5,cut).zones[0].block=="one");
    ordered=z;ordered.mode=Mode::first_available;ordered.entries[0].available=false;
    s.clear();advance({ordered},s,0,fade);ordered.entries[0].available=true;
    CHECK(advance({ordered},s,1,fade).zones[0].from_block=="two");
    reset(s,false,true);CHECK(!s["a"].fade && s["a"].shown==0);
    advance({z},s,4,fade);reset(s,true,false);CHECK(s.empty());
    CHECK(advance({z},s,5,fade).zones[0].block=="one");
    auto other=s;advance({},s,4,cut);CHECK(s.empty() && !other.empty());
    s.clear();auto stretched=cut;stretched.min_entry_seconds=20;advance({z},s,0,stretched);
    CHECK(advance({z},s,19,stretched).zones[0].block=="one");
    CHECK(advance({z},s,20,stretched).zones[0].block=="two");
    CHECK(budget_for(OutputKind::serial,{},0,0,160,480,0).shape==Shape::cut);
    CHECK(budget_for(OutputKind::serial,1000000,0,0,160,480,0).shape==Shape::reveal);
    CHECK(budget_for(OutputKind::serial,100000,0,0,160,480,0).shape==Shape::cut);
    CHECK(budget_for(OutputKind::serial,1000000,0,300,160,480,0).shape==Shape::cut);
    CHECK(budget_for(OutputKind::monitor,{},0,51,160,480,0).steps==2);
    CHECK(budget_for(OutputKind::monitor,{},0,101,160,480,0).shape==Shape::cut);
    CHECK(budget_for(OutputKind::companion,{},0,0,160,480,0).shape==Shape::cut);
    CHECK(budget_for(OutputKind::companion,{},0,0,160,480,2).steps==2);
    CHECK(serial_bytes(9,480)==16*480*2);
    CHECK(serial_bytes(160,480)==4*serial_bytes(40,480));
    CHECK(wake_at(0,{},false,false).min_gap==.5);
    CHECK(wake_at(0,{},false,true).min_gap==.1);
    CHECK(wake_at(0,{.2},true,false).min_gap==.05);
    CHECK(wake_at(0,{.2},true,false).wait_until==.2);
    // Referenz des bisherigen Skip-Pfads über 60 s, inklusive Wartefolge (0, 1.5, ...).
    for(int i=0;i<=120;++i) {
        double now=i*.5;bool due=i%30==0,minute=i==120,dirty=i==0;
        bool skip=!dirty && !due && !minute;
        bool reference=!skip;
        CHECK(should_render(dirty,due,false,0,false,minute,now,never)==reference);
        CHECK(wake_at(now,{},false,false).wait_until==now+1);
        CHECK(!should_render(false,false,false,0,false,false,now,now+.1));
        CHECK(should_render(false,false,false,0,false,false,now,now));
    }
    // Altes Skip-Prädikat als unabhängige Wahrheitstabelle aller Eingangskombinationen.
    for(int mask=0;mask<256;++mask) {
        bool dirty=mask&1,due=mask&2,live=mask&4,step=mask&8,size=mask&16,minute=mask&32;
        double now=mask&64 ? 2 : 0, deadline=mask&128 ? 1 : never;
        bool old_skip=!dirty && !due && !live && !step && !size && !minute;
        CHECK(should_render(dirty,due,live,step,size,minute,now,deadline)==(!old_skip || now>=deadline));
    }
    // Zwei gleichzeitig fällige Zonen: zweite wartet nur bis Übergangsende.
    s.clear();auto z2=z;z2.zone="b";
    advance({z,z2},s,0,fade);auto both=advance({z,z2},s,3,fade);
    CHECK(both.zones[0].from_block && !both.zones[1].from_block);
    CHECK(s["b"].shown==0);
    auto identical=advance({z,z2},s,3,fade);
    CHECK(identical.next_deadline==both.next_deadline && identical.zones[1].block==both.zones[1].block);
    auto follow=advance({z,z2},s,3.401,fade);CHECK(follow.zones[1].from_block.has_value());
    // Getrennte Ansichten haben eigene Fristen und Lebenszyklen.
    std::map<std::string,ZoneState> astate,bstate;
    auto ar=advance({z},astate,0,fade);auto br=advance({z},bstate,1,cut);
    CHECK(ar.next_deadline==3 && br.next_deadline==4);
    advance({},astate,2,cut);CHECK(astate.empty() && !bstate.empty());
    // Konkrete Referenzliste der alten Schleife ohne Wechselfeld über 60 s.
    std::vector<double> frames;
    double switched=0;
    for(double tick=0;tick<=60;tick+=1.5) {
        bool due=tick-switched>=15;
        if(should_render(tick==0,due,false,0,false,tick==60,tick,never)) {
            frames.push_back(tick);if(due)switched=tick;
        }
        CHECK(wake_at(tick+.5,{},false,false).wait_until==tick+1.5);
    }
    CHECK((frames==std::vector<double>{0,15,30,45,60}));
    // Review 4b: ein next in der wartenden Zone bleibt bis zur Freigabe erhalten.
    z.mode=Mode::rotate;z.entries={{"one",30,true},{"two",30,true}};z2=z;z2.zone="b";s.clear();
    advance({z,z2},s,0,fade);
    auto requested=advance({z,z2},s,1,fade,{"a","b"});
    CHECK(requested.zones[0].from_block.has_value());CHECK(s["b"].pending_next);
    CHECK(s["b"].shown==0);
    advance({z,z2},s,1.41,fade);CHECK(s["b"].shown==1 && !s["b"].pending_next);
    // Doppelte prepare-Auswertung bei derselben Uhrzeit dreht nicht ein zweites Mal.
    s.clear();advance({z},s,0,cut);advance({z},s,1,cut,{"a"});
    CHECK(advance({z},s,1,cut,{"a"}).zones[0].block=="two");
    z.mode=Mode::first_available;s.clear();advance({z},s,0,cut);
    CHECK(advance({z},s,1,cut,{"a"}).zones[0].block=="two");
    CHECK(advance({z},s,2,cut).zones[0].block=="two");
    // Ohne tap: first_available muss einen automatischen Wechsel auch nach Wartezeit ausführen.
    z.mode=Mode::rotate;z2=z;z2.zone="b";z2.mode=Mode::first_available;
    z2.entries[0].available=false;s.clear();advance({z,z2},s,0,fade);
    z2.entries[0].available=true;
    advance({z,z2},s,1,fade,{"a"});CHECK(s["b"].shown==1);
    advance({z,z2},s,1.41,fade);CHECK(s["b"].shown==0);
    std::printf("schedule: %d passed, %d failed\n",passed,failed);return failed?1:0;
}
