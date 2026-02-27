let devices = [];

async function renderDevices(){
  const ul = document.getElementById('devicesList');
  try {
    await loadDevices();
    console.log(devices);
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
    const res=await fetch('/api/wake',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({device:index})
    });
    
    if(!res.ok) throw new Error('Error');
    
  } catch(e) {console.error(e);} 
}

// SSE listeners
const evtSource = new EventSource('/events');

evtSource.addEventListener('status', event => {
  const data = JSON.parse(event.data);
  const statusElement = document.getElementById('status');
  updateFooterStatus(data);
});

evtSource.addEventListener('devices-changed', renderDevices);

async function updateStatus(){
  try {
    const res = await fetch('/api/wake/status');
  
    if(!res.ok) throw new Error('Error');
  
    const data = await res.json();  
    updateFooterStatus(data);

  } catch(e) {
    console.error(e);
  } 
}

async function loadDevices(){
  try{
    const res = await fetch('/api/devices');
    if(!res.ok) throw new Error('Failed to load devices');
    const data = await res.json();
    devices = data;
    }catch(e){console.error(e);}
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

renderDevices();