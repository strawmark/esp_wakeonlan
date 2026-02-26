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
    const data=await res.json();
    console.log(data);
  }catch(e){console.error(e);} 
}

async function updateStatus(){
  try{
    const res=await fetch('/api/status');
    if(!res.ok) throw new Error('Error');
    const data=await res.json();
    document.getElementById('status').textContent=`Status: ${data.sending?'sending':'idle'} (packets ${data.packetsSent})`;
  }catch(e){console.error(e);} 
}

renderDevices();
setInterval(updateStatus,2000);
