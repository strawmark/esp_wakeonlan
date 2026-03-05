let devices = [];

async function renderDevices(){
  const ul = document.getElementById('devicesList');
  try {
    await loadDevices();
    ul.innerHTML='';

    if (devices.length === 0) {
      const p = document.createElement('p');
      p.className = 'error';
      const li = document.createElement('li');
      p.textContent = 'Nessun dispositivo configurato';
      li.appendChild(p);
      ul.appendChild(li);
      return;
    }

    // Populate device list
    devices.forEach(d=>{
      const li=document.createElement('li');
      const btn=document.createElement('button');
      btn.textContent=`Accendi PC ${d.name}`;
      btn.onclick=()=>wakePC(d.index);
      li.appendChild(btn);
      ul.appendChild(li);
    });

  } catch(e) {
    const p = document.createElement('p');
    const li = document.createElement('li');
    p.textContent = 'Errore';
    li.appendChild(p);
    ul.appendChild(li);
    console.error(e);
  }

  updateStatus();
}

async function wakePC(index){
  try {
    const res=await fetch(`/api/devices/${index}/wake`,{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({device:index})
    });
    
    if(!res.ok) throw new Error('Error');
    
  } catch(e) {console.error(e);} 
}

async function updateDevice(index, data){
  try {
    const res=await fetch(`/api/devices/${index}`, {
      method:'PUT',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify(data)
    });
    
    if(!res.ok) throw new Error('Error');
    
  } catch(e) {console.error(e);}
}

async function patchDevice(index, data){
  try {
    const res=await fetch(`/api/devices/${index}`, {
      method:'PATCH',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify(data)
    });
    
    if(!res.ok) throw new Error('Error');
    
  } catch(e) {console.error(e);}
}

async function updateStatus(){
  try {
    const res = await fetch('/api/wake/status');
  
    if(!res.ok) throw new Error('Error');
  
    const data = await res.json();  
    updateFooterStatus(data);

  } catch(e) {console.error(e);} 
}

async function loadDevices(){
  try {
    const res = await fetch('/api/devices');
    
    if(!res.ok) throw new Error('Failed to load devices');
    
    const data = await res.json();
    devices = data;
    
  } catch(e) {console.error(e);}
}

function updateFooterStatus(status){
  const statusElement = document.getElementById('status');
    
  if(!status.sending){
    statusElement.textContent = `Status: pronto`
    statusElement.className = 'idle';
  } else {
    statusElement.textContent = `Status: inviando pacchetto a ${devices[status.currentIndex].name}`;
    statusElement.className = 'sending';
  }
}
//TODO
const urlParams = new URLSearchParams(window.location.search);
const deviceName = urlParams.get('device-name');
const originalUrl = urlParams.get('url');
  
if(originalUrl){

}

if(deviceName){

}

// SSE listeners
const evtSource = new EventSource('/events');

evtSource.addEventListener('status', event => updateFooterStatus(JSON.parse(event.data)));

evtSource.addEventListener('devices-changed', renderDevices);

// Initial rendering
renderDevices(); 