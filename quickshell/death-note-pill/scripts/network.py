#!/usr/bin/env python3
import json, subprocess, sys, re

def run(args, timeout=12):
    try:
        p=subprocess.run(args,capture_output=True,text=True,timeout=timeout)
        return p.returncode,p.stdout.strip(),p.stderr.strip()
    except Exception as e:return 1,'',str(e)

def wifi_list():
    _,state,_=run(['nmcli','radio','wifi'])
    _,data,_=run(['nmcli','-t','--escape','yes','-f','IN-USE,SSID,SIGNAL,SECURITY','device','wifi','list','--rescan','no'])
    items=[]
    for line in data.splitlines():
        fields=re.split(r'(?<!\\):',line)
        fields=[s.replace('\\:',':').replace('\\\\','\\') for s in fields]
        if len(fields)>=4 and fields[1]:
            items.append({'active':fields[0]=='*','name':fields[1], 'signal':fields[2], 'security':':'.join(fields[3:])})
    return {'enabled':state=='enabled','items':items}

def bluetooth_list():
    _,show,_=run(['bluetoothctl','show'])
    enabled='Powered: yes' in show
    _,devices,_=run(['bluetoothctl','devices'])
    items=[]
    for line in devices.splitlines():
        m=re.match(r'^Device ([0-9A-Fa-f:]{17}) (.*)$',line)
        if not m:continue
        addr,name=m.groups()
        _,info,_=run(['bluetoothctl','info',addr],timeout=4)
        items.append({'address':addr,'name':name,'paired':'Paired: yes' in info,'connected':'Connected: yes' in info})
    return {'enabled':enabled,'items':items}

def output(ok,msg=''):
    print(json.dumps({'ok':ok,'message':msg}))

def main():
    args=sys.argv[1:]
    if not args:output(False,'Missing action');return
    op=args[0]
    if op=='wifi-list':print(json.dumps(wifi_list()));return
    if op=='bt-list':print(json.dumps(bluetooth_list()));return
    if op=='wifi-scan':
        code,out,err=run(['nmcli','device','wifi','rescan'],timeout=20);output(code==0,out or err);return
    if op=='wifi-power' and len(args)>1:cmd=['nmcli','radio','wifi',args[1]]
    elif op=='wifi-connect' and len(args)>1:
        cmd=['nmcli','--wait','20','device','wifi','connect',args[1]]
        if len(args)>2 and args[2]:cmd+=['password',args[2]]
    elif op=='wifi-disconnect':
        code,dev,_=run(['nmcli','-t','-f','DEVICE,TYPE,STATE','device','status'])
        wifi=next((s.split(':')[0] for s in dev.splitlines() if ':wifi:connected' in s),'')
        if not wifi:output(False,'No connected Wi-Fi device');return
        cmd=['nmcli','device','disconnect',wifi]
    elif op=='bt-power' and len(args)>1:cmd=['bluetoothctl','power',args[1]]
    elif op=='bt-scan':
        # Scanning is bounded to 8 seconds; refresh list after completion.
        cmd=['bluetoothctl','--timeout','8','scan','on']
    elif op in ('bt-connect','bt-disconnect','bt-pair') and len(args)>1:
        if not re.fullmatch(r'[0-9A-Fa-f:]{17}',args[1]):output(False,'Invalid device address');return
        cmd=['bluetoothctl',{'bt-connect':'connect','bt-disconnect':'disconnect','bt-pair':'pair'}[op],args[1]]
    else:output(False,'Unknown action');return
    code,out,err=run(cmd,timeout=28)
    output(code==0,out or err or ('Completed' if code==0 else 'Command failed'))
if __name__=='__main__':main()
