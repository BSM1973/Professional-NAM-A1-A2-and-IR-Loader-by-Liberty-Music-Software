#!/usr/bin/env python3
"""Deterministic bounds checks for all active UI controls in the 5 pages.
This tests the intended C++ layout arithmetic at min/default/max dimensions.
It does not replace visual testing on macOS and Windows.
"""
from itertools import combinations

def overlap(a,b):
    return min(a[0]+a[2], b[0]+b[2]) > max(a[0],b[0]) and min(a[1]+a[3],b[1]+b[3]) > max(a[1],b[1])

def verify(width,height,page):
    all_rects={}
    def put(name,x,y,w,h):
        rect=(round(x),round(y),round(w),round(h))
        if rect[2] <= 0 or rect[3] <= 0:
            raise AssertionError((name,rect))
        if rect[0] < 0 or rect[1] < 0 or rect[0]+rect[2]>width or rect[1]+rect[3]>height:
            raise AssertionError(f'Off-screen {page} {name}: {rect} at {width}x{height}')
        all_rects[name]=rect
    ix,iy=16,16
    iw,ih=width-32,height-32
    iy+=76;ih-=76
    nav=(ix+8,iy+7,iw-16,48-14)
    navgap=8; navw=(nav[2]-4*navgap)//5
    for i in range(5):put(f'nav{i}',nav[0]+i*(navw+navgap),nav[1],navw,nav[3])
    iy+=48;ih-=48
    statusY=iy+ih-30
    put('status',ix+13,statusY+3,iw-26,24)
    ih-=30
    if page=='library':
        # Browser: whole page except 30 px status and navigation. All controls
        # are kept inside this one page; other pages' controls are hidden.
        bx,by,bw,bh=ix,iy+8,iw,ih-16
        r=(bx+22,by+22,bw-44,bh-44)
        rx,ry,rw,rh=r
        searchW=max(200,rw*35//100)
        put('folder',rx,ry,142,34)
        put('refresh',rx+152,ry,126,34)
        put('search',rx+290,ry,searchW,34)
        put('filter',rx+rw-166,ry,166,34)
        put('libraryPath',rx+2,ry+39,rw-4,26)
        ct=ry+74; cb=ry+rh-62
        listW=(rw-20)*58//100
        put('libraryList',rx,ct,listW,cb-ct)
        px=rx+listW+20; pw=rw-listW-20
        put('previewSurface',px,ct,pw,cb-ct)
        # Labels live inside the preview surface, so they are not siblings
        # eligible for intersection checks; verify their containment instead.
        for name,x,y,w,h in [
            ('detail',px+12,cb-61,pw-24,27),
            ('subdetail',px+12,cb-35,pw-24,23),
        ]:
            assert x >= px and x+w <= px+pw and y >= ct and y+h <= cb, (name,width,height)
        put('favourite',rx,ry+rh-44,180,35)
        put('loadA',rx+rw-300,ry+rh-44,145,35)
        put('loadB',rx+rw-145,ry+rh-44,145,35)
        for (a,ar),(b,br) in combinations(all_rects.items(),2):
            if overlap(ar,br):
                raise AssertionError(f'Overlapping {page} {width}x{height}: {a} {ar} and {b} {br}')
        return len(all_rects)
    footerY=iy+ih-170
    ih-=170
    masterY=footerY+6+46
    rowW=iw-60
    rowH=170-12-46
    w=(rowW-30)//4
    for i in range(4):
        x=ix+30+i*(w+10)
        put(f'footerlabel{i}',x,masterY,w,18)
        put(f'footerknob{i}',x+2,masterY+18,w-4,rowH-18)
    cx,cy,cw,ch=ix,iy+8,iw,ih-16
    leftW=(cw-16)//2
    lx,ly,lw,lh=cx,cy,leftW,ch
    rx,ry,rw,rh=cx+leftW+16,cy,cw-leftW-16,ch
    if page=='amp':
        rowTop=ly+lh-174
        artX=lx+22;artY=ly+54;artW=lw-44
        artH=max(180,rowTop-ly-64)
        innerX=artX+13;innerY=artY+6;innerW=artW-26;innerH=artH-12
        headHeight=(innerH-8)//2
        for i in range(2):
            put(f'ampVisual{i}',innerX,innerY+i*(headHeight+8),innerW,headHeight)
        inset=23;avail=lw-inset*2;fileW=min(161,avail//4)
        infoW=avail*30//100;choiceW=avail*33//100;photoW=avail*20//100
        clearW=avail-infoW-choiceW-photoW-12
        for i in range(2):
            y=rowTop+8+78*i
            put(f'load{i}',lx+inset,y,fileW,28)
            put(f'filename{i}',lx+inset+fileW+10,y,avail-fileW-10,28)
            under=y+33
            put(f'visualInfo{i}',lx+inset,under,infoW,27)
            put(f'visualChoice{i}',lx+inset+infoW+4,under,choiceW-4,27)
            put(f'photoButton{i}',lx+inset+infoW+choiceW+4,under,photoW-4,27)
            put(f'photoReset{i}',lx+inset+infoW+choiceW+photoW+4,under,clearW-4,27)
        gx,gy,gw,gh=rx+38,ry+115,rw-76,rh-230-35
        cw=(gw-8)//2; h=(gh-8)//2
        for i in range(4):
            x=gx+(i%2)*(cw+8);y=gy+(i//2)*(h+8)
            put(f'amplabel{i}',x,y,cw,19)
            put(f'ampknob{i}',x+3,y+19,cw-6,h-19)
        mode=(rw-108)//3
        for i in range(3):put(f'mode{i}',rx+28+i*(mode+14),ry+rh-90,mode,34)
        put('ampOn',rx+28,ry+rh-48,166,30)
    elif page=='cab':
        ph=max(188,lh//2-6)
        put('waveform',lx+19,ly+ph+59,lw-38,88)
        y=ly+ph+156
        mw=(lw-44-8)//2
        for i in range(2):put(f'micCanvas{i}',lx+20+i*(mw+8),y,mw,ly+lh-y-16)
        for i in range(2):
            row=ry+70+i*62
            put(f'loadIR{i}',rx+24,row,160,34)
            put(f'filenameIR{i}',rx+196,row-1,rw-222,36)
            put(f'miclabel{i}',rx+26,ry+205+i*54,142,20)
            put(f'micChoice{i}',rx+171,ry+205+i*54,rw-194,29)
        put('cabOn',rx+25,ry+170,150,30)
        put('phaseA',rx+186,ry+170,130,30)
        put('phaseB',rx+327,ry+170,130,30)
        gx,gy,gw,gh=rx+24,ry+332,rw-48,max(180,rh-351)
        cw=(gw-8)//2; h=(gh-8)//2
        for i in range(4):
            x=gx+(i%2)*(cw+8);y=gy+(i//2)*(h+8)
            put(f'micKnobLabel{i}',x,y,cw,19)
            put(f'micKnob{i}',x+3,y+19,cw-6,h-19)
    elif page=='fx':
        put('gate',lx+28,ly+94,180,32)
        put('boost',lx+225,ly+94,180,32)
        put('delay',rx+28,ry+94,180,32)
        gx,gy,gw,gh=lx+34,ly+138,lw-68,lh-276
        gy+=22;gh-=22
        put('gateLabel',gx,gy,gw,23)
        put('gateKnob',gx+110,gy+23,gw-220,170)
        w=(rw-76)//3
        for i in range(1,4):
            x=rx+30+(i-1)*(w+8);y=ry+172
            put(f'fxlabel{i}',x,y,w,22)
            put(f'fxknob{i}',x+3,y+22,w-6,178)
    else:
        put('ampOn',lx+34,ly+132,210,38)
        put('cabOn',lx+34,ly+187,210,38)
    for (a,ar),(b,br) in combinations(all_rects.items(),2):
        if overlap(ar,br):
            raise AssertionError(f'Overlapping {page} {width}x{height}: {a} {ar} and {b} {br}')
    return len(all_rects)

if __name__=='__main__':
    count=0
    for width,height in [(1370,930),(1480,950),(1560,980),(1600,1000),(1860,1200)]:
        for page in ('amp','cab','fx','settings','library'):
            count+=verify(width,height,page)
    print(f'Geometry test passed: {count} bounds, 5 pages x 5 sizes, no overlapping components.')
