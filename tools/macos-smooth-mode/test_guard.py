# SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception

from pathlib import Path
import tempfile,subprocess,os,time,signal,sys
binary=Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).parent/'awdl-guard').resolve()
def wait_for(fn, seconds=4):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        try:
            value=fn()
            if value:return value
        except (FileNotFoundError,ValueError,KeyError):pass
        time.sleep(.03)
    raise AssertionError('condition did not become true')
for role in ['server','client']:
 with tempfile.TemporaryDirectory(prefix='smooth-',dir='/private/tmp') as temp:
    p=Path(temp);log=p/'focus.log';control=p/'control';status=p/'status';net=p/'net';core=p/'core'
    log.write_text('[old] INFO: leaving computer\n');control.write_text('auto\n');net.write_text('up\n');core.write_text('1\n')
    process=subprocess.Popen([str(binary),'--dry-run','--role',role,'--uid',str(os.getuid()),'--log',str(log),'--control',str(control),'--status',str(status),'--fake-net',str(net),'--fake-core',str(core)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    def state():return dict(line.split('=',1) for line in status.read_text().splitlines())
    def ismode(m):return state()['mode']==m
    def append(msg):
        with log.open('a') as f:f.write('[x] INFO: '+msg+'\n');f.flush()
    enter='leaving computer' if role=='server' else 'entering computer'
    leave='entering computer' if role=='server' else 'leaving computer'
    try:
        wait_for(lambda:ismode('waiting')); assert net.read_text().strip()=='up'
        append(enter);wait_for(lambda:ismode('smooth'));assert net.read_text().strip()=='down'
        count=int(state()['down_count']);net.write_text('up\n')
        wait_for(lambda:int(state()['down_count'])>count);assert net.read_text().strip()=='down'
        append(leave);wait_for(lambda:ismode('allowed'));assert net.read_text().strip()=='up'
        append(enter);wait_for(lambda:ismode('smooth'))
        control.write_text('pause_until='+str(int(time.time())+2)+'\n')
        wait_for(lambda:ismode('airdrop'));assert net.read_text().strip()=='up'
        wait_for(lambda:ismode('smooth'),5);assert net.read_text().strip()=='down'
        control.write_text('disabled\n');wait_for(lambda:ismode('disabled'));assert net.read_text().strip()=='up'
        control.write_text('auto\n');wait_for(lambda:ismode('smooth'))
        log.rename(p/'old.log');log.write_text('')
        wait_for(lambda:ismode('waiting'));assert net.read_text().strip()=='up'
        append(enter);wait_for(lambda:ismode('smooth'))
        core.write_text('0\n');wait_for(lambda:ismode('deskflow-stopped'));assert net.read_text().strip()=='up'
        core.write_text('1\n');wait_for(lambda:ismode('waiting'))
        append(enter);wait_for(lambda:ismode('smooth'))
        if role=='server':
            append('new client disconnected')
            control.write_text('disabled\n');wait_for(lambda:ismode('disabled'))
            assert state()['focus_known']=='1' and state()['client_active']=='1'
            control.write_text('auto\n');wait_for(lambda:ismode('smooth'))
            assert net.read_text().strip()=='down'
        append('disconnected');wait_for(lambda:ismode('waiting'));assert net.read_text().strip()=='up'
        append(enter);wait_for(lambda:ismode('smooth'))
        # A symlinked log must fail open, not be followed by the privileged worker.
        log.unlink();log.symlink_to(p/'old.log')
        wait_for(lambda:ismode('waiting'));assert net.read_text().strip()=='up'
        log.unlink();log.write_text('');time.sleep(.15)
        append(enter);wait_for(lambda:ismode('smooth'))
        # Recover a crashed worker's owned-down journal even if the interface
        # is temporarily unreadable during restart. Keep ownership until retry.
        process.kill();process.wait(timeout=3)
        assert net.read_text().strip()=='down' and state()['owned_down']=='1'
        net.write_text('error\n')
        process=subprocess.Popen([str(binary),'--dry-run','--role',role,'--uid',str(os.getuid()),'--log',str(log),'--control',str(control),'--status',str(status),'--fake-net',str(net),'--fake-core',str(core)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        wait_for(lambda:ismode('error'))
        assert state()['owned_down']=='1' and state()['restore_up']=='1'
        net.write_text('down\n')
        wait_for(lambda:ismode('waiting'))
        assert net.read_text().strip()=='up' and state()['owned_down']=='0'
        append(enter);wait_for(lambda:ismode('smooth'))
        process.terminate();process.wait(timeout=3)
        assert net.read_text().strip()=='up' and state()['mode']=='stopped'
        print('PASS',role,': historical focus ignored; focus direction; OS re-enable suppression; host return; AirDrop pause/expiry; disable; rotation; core loss/restart; disconnect; unrelated handshake ignored; symlink rejection; crash journal and failed-restore retry; stop restoration')
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=3)
print('No real network interfaces were read or changed during these simulations.')
