const urlParams = new URLSearchParams(window.location.search);
const deviceName = urlParams.get('device-name');
const originalUrl = urlParams.get('url');

let devices = [];

if(originalUrl){
  const returnButton = document.getElementById("return");
  returnButton.innerHTML = `Riprova a raggiungere <strong>${originalUrl.split("/")[2]}</strong>`;
  returnButton.parentElement.style.display = 'inline';
  returnButton.setAttribute("href", originalUrl);
}

async function renderDevices(){
  const ul = document.getElementById('devicesList');
  try {
    await loadDevices();
    ul.innerHTML='';

    let displayDevices = [...devices]; 
    if (deviceName) {
      displayDevices = displayDevices.filter(d => d.name === deviceName);
    }

    if (displayDevices.length === 0) {
      const p = document.createElement('p');
      p.className = 'error';
      const li = document.createElement('li');
      p.textContent = deviceName ? 'Dispositivo non presente' : 'Nessun dispositivo configurato';      li.appendChild(p);
      ul.appendChild(li);
      return;
    }

    // Populate device list

    displayDevices.forEach(d=>{
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
    const targetDevice = devices.find(d => d.index === status.currentIndex);
    console.log(targetDevice);
    statusElement.textContent = `Status: invio segnale di wake a ${targetDevice? targetDevice.name : "dispositivo non riconosciuto"}`;
    statusElement.className = 'sending';
  }
}

// SSE listeners
const sseUrl = `/events?${urlParams.toString()}&t=${Date.now()}`;
const evtSource = new EventSource(sseUrl);
//const evtSource = new EventSource('/events');

evtSource.addEventListener('status', event => updateFooterStatus(JSON.parse(event.data)));

evtSource.addEventListener('devices-changed', renderDevices);

evtSource.onerror = (err) => {
  console.error("SSE Error:", err);
  if (evtSource.readyState === EventSource.CLOSED) {
    console.log("Connessione persa, tentativo di ripristino...");
  }
};

// Initial rendering
renderDevices(); 