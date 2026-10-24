// Headless browser checks against synthetic fixtures from app_companion_integration.py.
// Run with Playwright available on NODE_PATH; no production app or private screenshots.
const {chromium} = require('playwright');
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname,'../..');
const html = fs.readFileSync(path.join(root,'cpp/res/webui.html'),'utf8');
const fixture = JSON.parse(fs.readFileSync(path.join(root,'cpp/build/ui-config-fixture.json'),'utf8'));
let checks=0;
function check(value,label) { if(!value)throw Error(label); checks++; }
(async()=>{
  const browser=await chromium.launch({channel:'msedge',headless:true});
  try {
    for(const lang of ['de','en']) {
      const page=await browser.newPage({viewport:{width:1300,height:1000}}), errors=[];
      page.on('pageerror',e=>errors.push(e.message));
      const data=structuredClone(fixture);data.config.display.output='serial';data.config.companion.enabled=false;
      data.first_run=false;data.config.ui.language=lang;
      await page.route('http://127.0.0.1:18891/**',async route=>{
        const url=new URL(route.request().url());
        if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
        let result={};
        if(url.pathname==='/api/config')result=data;
        if(url.pathname==='/api/display/options')result={size:[480,320],monitors:[],touch:{available:false},presets:{}};
        if(url.pathname==='/api/companion/status')result={enabled:false};
        if(url.pathname==='/api/pages')result={index:0,total:2,visible:1,current:['system'],can_step:true};
        return route.fulfill({contentType:'application/json',body:JSON.stringify(result)});
      });
      await page.goto('http://127.0.0.1:18891/?lang='+lang);
      await page.locator('#nav').getByRole('button',{name:'Companion',exact:true}).click();
      const title=lang==='en'?'Independent Companion display':'Unabhängige Companion-Anzeige';
      check(await page.getByText(title,{exact:true}).count()===0,lang+': independent controls hidden when off');
      await page.locator('#main input[type=checkbox]').first().check();
      check(await page.getByText(title,{exact:true}).isVisible(),lang+': enabling reveals independent controls');
      check(await page.locator('#main input[type=range]').count()===1,lang+': own companion interval');
      await page.locator('#main input[aria-label="System"]').uncheck();
      check(await page.evaluate(()=>!cfg.companion.pages.includes('system') && cfg.display.visible_pages.includes('system')),lang+': selection belongs to companion only');
      await page.screenshot({path:path.join(root,'cpp/build/companion-settings-'+lang+'.png'),fullPage:true});
      await page.locator('#nav').getByRole('button',{name:'Display',exact:true}).click();
      const displayTitle=lang==='en'?'Pages on USB display / monitor':'Seiten auf USB-Display / Bildschirm';
      check(await page.getByText(displayTitle,{exact:true}).isVisible(),lang+': primary selection visible with companion');
      await page.locator('#main input[aria-label="System"]').uncheck();
      check(await page.evaluate(()=>!cfg.display.visible_pages.includes('system')),lang+': primary selection stored separately');
      await page.evaluate(()=>{cfg.companion.enabled=false;render();});
      check(await page.getByText(displayTitle,{exact:true}).count()===0,lang+': extra primary controls hidden when off');
      await page.locator('#nav').getByRole('button',{name:'Companion',exact:true}).click();
      await page.locator('#main input[type=checkbox]').first().check();
      check(await page.getByText(title,{exact:true}).isVisible(),lang+': re-enabling keeps the UI usable');
      check(errors.length===0,lang+': no JavaScript errors: '+errors.join(';'));
      await page.close();
    }
    // Compile every embedded/template guide script, including the new guide step.
    for(const file of ['cpp/res/ANLEITUNG.html','cpp/res/GUIDE.html','anleitung/anleitung.template.html','anleitung/anleitung.template.en.html']) {
      const content=fs.readFileSync(path.join(root,file),'utf8');
      for(const m of content.matchAll(/<script[^>]*>([\s\S]*?)<\/script>/gi))new Function(m[1]);
      check(content.includes('Companion:'),file+': Companion guide included');
    }
    console.log(checks+' browser checks passed');
  } finally {await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
