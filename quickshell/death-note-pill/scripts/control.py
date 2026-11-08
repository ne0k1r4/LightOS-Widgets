#!/usr/bin/env python3
import os,sys,subprocess,json,pathlib,random,re
HOME=pathlib.Path.home()
def run(args):
    try: return subprocess.run(args,text=True,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,timeout=8).stdout.strip()
    except Exception: return ''
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".webp"}
VIDEO_EXTS = {".mp4", ".webm", ".mkv", ".mov"}

WALLPAPER_ROOT = HOME / ".config" / "Light" / "wallpaper"

# Personal installation only. Do not publish this extra directory.
EXTRA_WALLPAPER_ROOTS = []

def wallpapers():
    roots = [WALLPAPER_ROOT] + EXTRA_WALLPAPER_ROOTS
    found = set()
    for base in roots:
        if base.is_dir():
            for p in base.rglob("*"):
                if p.is_file() and p.suffix.lower() in IMAGE_EXTS | VIDEO_EXTS:
                    found.add(str(p.resolve()))
    return sorted(found)

def current():
    cache = HOME / ".cache" / "lightos-gothic-wallpaper"
    return cache.read_text().strip() if cache.exists() else ""

def setwall(path):
    import time
    import shutil

    target = Path(path).resolve()
    if str(target) not in wallpapers():
        print("Wallpaper not in gallery", file=sys.stderr)
        return

    is_video = target.suffix.lower() in VIDEO_EXTS

    if is_video:
        if not shutil.which("mpvpaper"):
            print("mpvpaper not installed", file=sys.stderr)
            return

        subprocess.run(["pkill", "-x", "mpvpaper"], check=False)
        time.sleep(0.25)

        subprocess.Popen([
            "mpvpaper",
            "-o", "loop audio=auto volume=50 no-border keepaspect=no hwdec=auto",
            "*", str(target)
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
           start_new_session=True)
    else:
        if not shutil.which("awww"):
            print("awww not installed", file=sys.stderr)
            return

        subprocess.run(["pkill", "-x", "mpvpaper"], check=False)

        subprocess.Popen([
            "awww", "img", str(target),
            "--transition-type", "wipe",
            "--transition-duration", "0.7"
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
           start_new_session=True)

    cache = HOME / ".cache" / "lightos-gothic-wallpaper"
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text(str(target))

    color_script = HOME / ".config/waybar/Scripts/wallpaper-colors.py"
    if color_script.is_file():
        subprocess.Popen(
            ["python3", str(color_script), str(target)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True
        )

# LIGHTOS_V113_THUMBNAILS

def wallpaper_thumbnails():
    import hashlib
    from urllib.parse import quote
    cache = HOME / '.cache' / 'lightos-gothic-thumbnails'
    cache.mkdir(parents=True, exist_ok=True)
    result = {}
    for item in wallpapers():
        path = pathlib.Path(item)
        if path.suffix.lower() not in VIDEO_EXTS:
            continue
        key = hashlib.sha256(str(path).encode('utf-8')).hexdigest()[:24]
        target = cache / (key + '.jpg')
        try:
            stale = not target.exists() or target.stat().st_mtime < path.stat().st_mtime
            if stale:
                temp = cache / (key + '.tmp.jpg')
                proc = subprocess.run(
                    ['ffmpeg', '-hide_banner', '-loglevel', 'error', '-nostdin',
                     '-ss', '00:00:01', '-i', str(path), '-frames:v', '1',
                     '-vf', 'scale=320:180:force_original_aspect_ratio=decrease',
                     '-y', str(temp)], timeout=12, stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL)
                if proc.returncode == 0 and temp.is_file():
                    temp.replace(target)
                else:
                    temp.unlink(missing_ok=True)
            if target.is_file():
                result[item] = target.as_uri()
        except (OSError, subprocess.TimeoutExpired):
            pass
    return result

cmd=sys.argv[1] if len(sys.argv)>1 else 'status'
if cmd=='wallpaper-thumbnails': print(json.dumps(wallpaper_thumbnails()))
elif cmd=='wallpapers': print(json.dumps(wallpapers()))
elif cmd=='next-wallpaper':
    a=wallpapers()
    if a:setwall(a[(a.index(current())+1)%len(a)] if current() in a else a[0])
elif cmd=='set-wallpaper' and len(sys.argv)>2:setwall(sys.argv[2])
elif cmd=='status':
    battery='--'; charging=False
    for p in pathlib.Path('/sys/class/power_supply').glob('BAT*'):
        try:
            battery=(p/'capacity').read_text().strip(); charging=(p/'status').read_text().strip()=='Charging';break
        except Exception:pass
    info=run(['playerctl','metadata','--format','{{title}}|||{{artist}}|||{{mpris:artUrl}}'])
    playback=run(['playerctl','status'])
    rawvol=run(['wpctl','get-volume','@DEFAULT_AUDIO_SINK@'])
    volmatch=re.search(r'(\d+(?:\.\d+)?)',rawvol)
    volume_percent=round(float(volmatch.group(1))*100) if volmatch else 50
    unread=run(['swaync-client','-c'])
    notification_count=int(unread) if unread.isdigit() else 0
    # A compatible notification history bridge may write this JSON file.
    # If absent, keep the honest unread-count-only fallback.
    notifications=[]
    try:
        history=HOME/'.local/state/lightos-notifications/history.json'
        if history.is_file():
            parsed=json.loads(history.read_text(encoding='utf-8'))
            if isinstance(parsed,list):
                for item in parsed[:10]:
                    if isinstance(item,dict):
                        notifications.append({
                            'app': str(item.get('app') or item.get('title') or 'Notification')[:60],
                            'summary': str(item.get('summary') or item.get('title') or '')[:120],
                            'body': str(item.get('body') or '')[:180]
                        })
    except (ValueError,OSError,TypeError):pass
    preview=''
    try:
        cache=HOME/'.cache/lightos-gothic-notification.txt'
        if cache.exists(): preview=cache.read_text(encoding='utf-8')[:90].strip()
    except Exception: pass
    parts=(info.split('|||')+['','',''])[:3]
    # Leave artwork URLs untouched; QML handles file:// and remote MPRIS sources.
    if parts[2].startswith('file://'):
        from urllib.parse import unquote
        parts[2] = 'file://' + unquote(parts[2][7:])

    if not preview and notifications:
        latest = notifications[0]
        preview = (
            latest.get('body')
            or latest.get('summary')
            or latest.get('app')
            or ''
        )[:180]

    print(json.dumps({'battery':battery,'charging':charging,'title':parts[0] or 'Nothing playing','artist':parts[1], 'cover':parts[2], 'wifi':run(['nmcli','-t','-f','WIFI','general']),'ssid':run(['nmcli','-t','-f','ACTIVE,SSID','dev','wifi']).splitlines()[:2], 'volume':rawvol,'volumePercent':volume_percent,'playing':playback=='Playing','notificationCount':notification_count,'notificationPreview':preview,'notifications':notifications,'bluetooth':run(['bluetoothctl','show'])[:100]}))
elif cmd=='wifi-toggle':run(['nmcli','radio','wifi','off' if run(['nmcli','radio','wifi'])=='enabled' else 'on'])
elif cmd=='bluetooth-toggle':run(['bluetoothctl','power','off' if 'Powered: yes' in run(['bluetoothctl','show']) else 'on'])
elif cmd=='volume' and len(sys.argv)>2:run(['wpctl','set-volume','@DEFAULT_AUDIO_SINK@',str(max(0,min(100,int(sys.argv[2]))))+'%'])
elif cmd=='media' and len(sys.argv)>2 and sys.argv[2] in ('previous','next','play-pause'):run(['playerctl',sys.argv[2]])

elif cmd=='notifications-open':
    run(['swaync-client','-t'])
