"""Run an isolated Windows app with --no-lcd; never touches COM7 or port 8765.

Usage: python cpp/tests/app_companion_integration.py
Uses only the standard library. Pairing keys stay in memory / ignored build files.
"""
import base64
import json
import os
from pathlib import Path
import socket
import ssl
import struct
import subprocess
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'cpp/build'
checks = []

def check(ok, label):
    if not ok: raise AssertionError(label)
    checks.append(label)

def wait_for(fn, seconds=12):
    end = time.monotonic()+seconds
    while time.monotonic()<end:
        try:
            value=fn()
            if value: return value
        except (OSError, ValueError): pass
        time.sleep(.15)
    raise AssertionError('Condition timed out')

class WS:
    def __init__(self, host, port, fingerprint):
        ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT);ctx.minimum_version=ssl.TLSVersion.TLSv1_2;ctx.check_hostname=False;ctx.verify_mode=ssl.CERT_NONE
        self.s=ctx.wrap_socket(socket.create_connection((host,port),timeout=4),server_hostname=host)
        import hashlib
        check(hashlib.sha256(self.s.getpeercert(binary_form=True)).hexdigest()==fingerprint.replace(':','').lower(),'TLS fingerprint matches isolated server')
        self.s.sendall((f'GET / HTTP/1.1\r\nHost: {host}:{port}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: '+base64.b64encode(os.urandom(16)).decode()+'\r\n\r\n').encode())
        self.buf=b''
        while b'\r\n\r\n' not in self.buf:self.buf+=self.s.recv(4096)
        head,self.buf=self.buf.split(b'\r\n\r\n',1);check(b'101' in head,'WebSocket upgrade')
    def take(self,n):
        while len(self.buf)<n:
            b=self.s.recv(65536)
            if not b:raise OSError('closed')
            self.buf+=b
        b,self.buf=self.buf[:n],self.buf[n:];return b
    def send(self,msg):
        data=json.dumps(msg).encode();mask=os.urandom(4)
        head=bytes([0x81,0x80|len(data)]) if len(data)<126 else bytes([0x81,0xfe])+struct.pack('!H',len(data))
        self.s.sendall(head+mask+bytes(x^mask[i%4] for i,x in enumerate(data)))
    def recv(self):
        a,b=self.take(2);n=b&127
        if n==126:n=struct.unpack('!H',self.take(2))[0]
        elif n==127:n=struct.unpack('!Q',self.take(8))[0]
        data=self.take(n)
        if a&15==1:
            msg=json.loads(data)
            if msg['t']=='ping':self.send({'t':'pong'})
            return msg
        return {'t':'binary','data':data}
    def ready(self,w,h):
        until=time.monotonic()+12
        while time.monotonic()<until:
            m=self.recv()
            if m.get('t')=='ready' and (m['w'],m['h'])==(w,h):return
        raise AssertionError('Native ready not received')
    def close(self):self.s.close()

def run_case(enabled, network=False, mode='navigation'):
    # OS-selected free ports, explicitly excluding all known app ports.
    ports=[]
    for _ in range(2):
        with socket.socket() as s:s.bind(('127.0.0.1',0));ports.append(s.getsockname()[1])
    ui,phone=ports;check(ui!=phone and not {ui,phone}&{8765,8777,8799},'Isolated test ports')
    work=BUILD/f'integration-{enabled}-{network}-{mode}';work.mkdir(exist_ok=True)
    config=work/'config.integration.toml'
    primary_pages = '"system"' if mode=='selection' else '"system", "friends"'
    phone_pages = '"friends"' if mode=='selection' else '"system", "friends"'
    config.write_text(f'''[display]
output = "{'network' if network else 'serial'}"
visible_pages = [{primary_pages}]
page_seconds = 600
[companion]
enabled = {str(enabled).lower()}
port = {phone}
pages = [{phone_pages}]
page_seconds = {3 if mode=='timer' else 600}
[ui]
port = {ui}
setup_done = true
update_check = false
[discord]
enabled = false
[friends]
enabled = true
include_discord = false
[system]
enabled = true
''',encoding='utf-8')
    base=f'http://127.0.0.1:{ui}'
    def api(path,body=None,raw=False):
        req=urllib.request.Request(base+path, data=None if body is None else json.dumps(body).encode(), headers={'X-FL':'1','Content-Type':'application/json'})
        with urllib.request.urlopen(req,timeout=4) as r:data=r.read()
        return data if raw else json.loads(data)
    env=dict(os.environ,FUCHSLABS_INSTANCE=f'-companion-integration-{ui}')
    args=[str(BUILD/'FuchsLabs-USB-Display.exe'),'--run','--config',str(config),'--log',str(work/'test.log')]
    if not network:args+=['--no-lcd']
    p=subprocess.Popen(args,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    ws=None
    try:
        wait_for(lambda:api('/api/pages').get('total')==(1 if mode=='selection' else 2))
        check(p.poll() is None,'App starts without touching physical display')
        if not enabled and not network:
            check(not api('/api/companion/status')['enabled'],'Companion disabled, ordinary app remains stable')
            return
        status=wait_for(lambda:(x if (x:=api('/api/companion/status')).get('running') else None))
        if mode=='selection':
            wait_for(lambda:api('/api/companion/pages').get('total')==1)
            check(api('/api/pages')['current']==['system'],'Primary selection contains only system')
            check(api('/api/companion/pages')['current']==['friends'],'Companion selection contains only friends')
            return
        pair=api('/api/companion/pair',{});check(pair['ok'],'Local test pairing')
        ws=WS(status['addresses'][0],phone,status['fingerprint'])
        ws.send({'t':'hello','v':1,'pin':pair['pin'],'w':960,'h':720,'name':'Integration Example'})
        ws.ready(960,720)
        png=wait_for(lambda:(b if struct.unpack('!II',(b:=api('/api/companion/preview.png',raw=True))[16:24])==(960,720) else None))
        check(bool(png),'Real app renders native 960x720 pixels')
        if mode=='timer':
            primary=api('/api/pages')['index'];before=api('/api/companion/pages')['index']
            wait_for(lambda:api('/api/companion/pages')['index']!=before)
            check(api('/api/pages')['index']==primary,'Independent 3-second companion timer leaves primary unchanged')
            return
        fixture=api('/api/config')
        fixture.pop('config_file',None)
        (BUILD/'ui-config-fixture.json').write_text(json.dumps(fixture),encoding='utf-8')
        primary=api('/api/pages')['index'];before=api('/api/companion/pages')['index']
        ws.send({'t':'swipe','dir':'left'})
        wait_for(lambda:api('/api/companion/pages')['index']!=before)
        check(network or api('/api/pages')['index']==primary,'Phone swipe leaves primary view unchanged')
        if not network:
            before=api('/api/companion/pages')['index'];api('/api/pages/step',{'delta':1})
            wait_for(lambda:api('/api/pages')['index']!=primary)
            check(api('/api/companion/pages')['index']==before,'Primary navigation leaves phone unchanged')
        ws.send({'t':'resize','w':720,'h':960});ws.ready(720,960)
        check(True,'Portrait rotation announces actual frame dimensions')
        ws.send({'t':'resize','w':1520,'h':720});ws.ready(1520,720)
        wait_for(lambda:struct.unpack('!II',api('/api/companion/preview.png',raw=True)[16:24])==(1520,720))
        check(True,'Redmi landscape resolution rendered without fixed config')
        if not network:
            check(struct.unpack('!II',api('/api/preview.png',raw=True)[16:24])==(480,320),'Primary frame remains 480x320 during phone rotation')
    finally:
        if ws:ws.close()
        try:api('/api/quit',{})
        except OSError:pass
        try:p.wait(timeout=12)
        except subprocess.TimeoutExpired:p.terminate();p.wait(timeout=5)
        check(p.returncode==0,'Isolated app exits cleanly')

if __name__=='__main__':
    run_case(False)
    run_case(True)
    run_case(False,True)
    run_case(True,mode='selection')
    run_case(True,mode='timer')
    print(f'{len(checks)} app integration checks passed')
    (BUILD/'app-integration-result.json').write_text(json.dumps({'checks':checks},indent=2),encoding='utf-8')
