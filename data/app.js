const devices = [
  {index:0,name:"Glottis"},
  {index:1,name:"BlueCasket"},
  {index:2,name:"Guybrush"},
  {index:3,name:"Myu59"}
];

function renderDevices(){
  const ul=document.getElementById('devices');
  ul.innerHTML='';
  devices.forEach(d=>{
    const li=document.createElement('li');
    const btn=document.createElement('button');
    btn.textContent=`Accendi PC ${d.name}`;
    btn.onclick=()=>wakePC(d.index);
    li.appendChild(btn);
    ul.appendChild(li);
  });
}

async function wakePC(index){
  try{
    const res=await fetch('/api/wake',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({device:index})
    });
    
    if(!res.ok) throw new Error('Error');
    
  } catch(e) {console.error(e);} 
}

// SSE listener – updates UI in real time
const evtSource = new EventSource('/events');
evtSource.addEventListener('status', event => {
    const data = JSON.parse(event.data);
    const statusElement = document.getElementById('status');

    if (!data.sending) {
        statusElement.textContent = 'Status: pronto';
        statusElement.className = 'idle';
    } else {
        statusElement.textContent = `Status: inviando pacchetto a ${devices[data.currentIndex].name}`;
        statusElement.className = 'sending';
    }
});

async function updateStatus(){
  try{
    const res = await fetch('/api/status');
  
    if(!res.ok) throw new Error('Error');
  
    const data = await res.json();  
    const statusElement = document.getElementById('status');
    
    if(!data.sending){
      statusElement.textContent = `Status: pronto`
      statusElement.className = 'idle';
    } else {
      statusElement.textContent = `Status: inviando pacchetto a ${devices[data.currentIndex].name}`;
      statusElement.className = 'sending';
    }   

  } catch(e) {
    console.error(e);
  } 
}

renderDevices();
//setInterval(updateStatus,2000);
