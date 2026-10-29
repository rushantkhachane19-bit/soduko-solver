const boardEl=document.getElementById('board');
const sizeSelect=document.getElementById('sizeSelect');
const difficultySelect=document.getElementById('difficultySelect');
const puzzleSelect=document.getElementById('puzzleSelect');
const puzzleLabel=document.getElementById('puzzleLabel');
const solveBtn=document.getElementById('solveBtn');
const loadBtn=document.getElementById('loadBtn');
const clearBtn=document.getElementById('clearBtn');
const messageEl=document.getElementById('message');
const badgeEl=document.getElementById('boardBadge');

let currentSize=9, boxRows=3, boxCols=3, current=[], givens=[], solutionMode=false;
const sizes={4:[2,2],6:[2,3],9:[3,3],16:[4,4]};
const symbolFor=v=>v===0?'':v<=9?String(v):String.fromCharCode(55+v);
function valueFor(ch){if(!ch)return 0;ch=ch.toUpperCase();if(ch>='1'&&ch<='9')return Number(ch);if(ch>='A'&&ch<='G')return ch.charCodeAt(0)-55;return 0;}
function makeSolvedGrid(n,br,bc,variant=0){
  const nums=Array.from({length:n},(_,i)=>i);
  // Deterministic row/column/symbol permutations preserve valid Sudoku structure.
  function shuffleGroups(arr,groupSize,seed){
    const groups=[];for(let i=0;i<arr.length;i+=groupSize)groups.push(arr.slice(i,i+groupSize));
    if(seed%2)groups.reverse();
    groups.forEach((g,i)=>{if((seed+i)%2)g.reverse();});
    return groups.flat();
  }
  const rows=shuffleGroups(nums,br,variant+1);
  const cols=shuffleGroups(nums,bc,variant+2);
  const shift=variant%n;
  const out=[];
  for(let ri=0;ri<n;ri++){
    const r=rows[ri], row=[];
    for(let ci=0;ci<n;ci++){
      const c=cols[ci];
      const val=(r*bc+Math.floor(r/br)+c+shift)%n+1;
      row.push(val);
    }
    out.push(row);
  }
  // A cyclic symbol permutation also preserves validity.
  const map=v=>((v-1+variant)%n)+1;
  return out.map(row=>row.map(map));
}
function makePuzzle(n, difficulty, variant){
  const [br,bc]=sizes[n];const solved=makeSolvedGrid(n,br,bc,variant+1);
  const puzzle=solved.map(row=>row.slice());
  const total=n*n;
  const keepRatio=difficulty==='Easy'?.72:difficulty==='Medium'?.52:.38;
  const target=Math.floor(total*keepRatio);
  // Keep a deterministic set of clues distributed across the grid.
  const cells=[];
  for(let i=0;i<total;i++)cells.push(i);
  cells.sort((a,b)=>((a*37+variant*19)%total)-((b*37+variant*19)%total));
  const keep=new Set(cells.slice(0,target));
  for(let r=0;r<n;r++)for(let c=0;c<n;c++)if(!keep.has(r*n+c))puzzle[r][c]=0;
  return puzzle;
}
function puzzleList(){
  const d=difficultySelect.value,n=currentSize;
  if(d==='Custom')return [];
  return [0,1,2].map(i=>({label:`Puzzle ${i+1}`,board:makePuzzle(n,d,i+(d==='Easy'?0:d==='Medium'?3:6))}));
}
function resetStats(){
  ['recursiveCalls','placements','backtracks','elapsed'].forEach(id=>document.getElementById(id).textContent='—');
  const state=document.getElementById('solutionState');state.className='solution-state';
  state.innerHTML='<span class="state-icon">◎</span><div><b>Waiting for puzzle</b><small>Results appear here after analysis.</small></div>';
}
function setMessage(t,error=false){messageEl.textContent=t;messageEl.className='message'+(error?' error':'');}
function renderBoard(){
  boardEl.innerHTML='';boardEl.style.setProperty('--n',currentSize);
  for(let r=0;r<currentSize;r++)for(let c=0;c<currentSize;c++){
    const inp=document.createElement('input');inp.className='cell';inp.type='text';inp.maxLength=1;
    inp.autocomplete='off';inp.spellcheck=false;inp.inputMode='text';inp.dataset.r=r;inp.dataset.c=c;
    inp.setAttribute('aria-label',`Row ${r+1}, column ${c+1}`);
    inp.value=symbolFor(current[r][c]);
    if(givens[r][c]){inp.classList.add('given');inp.readOnly=true;}
    if((c+1)%boxCols===0&&c!==currentSize-1)inp.classList.add('box-right');
    if((r+1)%boxRows===0&&r!==currentSize-1)inp.classList.add('box-bottom');
    inp.addEventListener('input',()=>{
      const raw=inp.value.slice(-1).toUpperCase();
      const val=valueFor(raw);
      if(raw && (!val||val>currentSize)){inp.value='';current[r][c]=0;}
      else {inp.value=raw;current[r][c]=val;}
      solutionMode=false;inp.classList.remove('invalid');resetStats();
      badgeEl.textContent='Edited';setMessage('Puzzle edited. Click Solve & Analyze to check it.');
      updateHighlights(r,c);
    });
    inp.addEventListener('focus',()=>updateHighlights(r,c));
    inp.addEventListener('keydown',e=>{
      let rr=r,cc=c;
      if(e.key==='ArrowUp')rr--;else if(e.key==='ArrowDown')rr++;
      else if(e.key==='ArrowLeft')cc--;else if(e.key==='ArrowRight')cc++;else return;
      e.preventDefault();rr=Math.max(0,Math.min(currentSize-1,rr));cc=Math.max(0,Math.min(currentSize-1,cc));
      boardEl.querySelector(`[data-r="${rr}"][data-c="${cc}"]`).focus();
    });
    boardEl.appendChild(inp);
  }
}
function updateHighlights(row,col){
  const val=current[row][col];
  boardEl.querySelectorAll('.cell').forEach(el=>{
    const r=+el.dataset.r,c=+el.dataset.c;
    el.classList.toggle('related',r===row||c===col||(Math.floor(r/boxRows)===Math.floor(row/boxRows)&&Math.floor(c/boxCols)===Math.floor(col/boxCols)));
    el.classList.toggle('same-number',val!==0&&current[r][c]===val);
  });
}
function loadSelected(){
  if(difficultySelect.value==='Custom'){
    current=Array.from({length:currentSize},()=>Array(currentSize).fill(0));
    givens=Array.from({length:currentSize},()=>Array(currentSize).fill(0));
    puzzleLabel.style.display='none';badgeEl.textContent='Custom puzzle';
    renderBoard();resetStats();setMessage('Enter your own puzzle. Use 1–9 and A–G for 16×16; leave empty cells blank.');
    return;
  }
  const list=puzzleList();
  const requestedIndex=Number(puzzleSelect.value)||0;
  puzzleSelect.innerHTML='';
  list.forEach((p,i)=>{const opt=document.createElement('option');opt.value=i;opt.textContent=p.label;puzzleSelect.appendChild(opt);});
  puzzleSelect.value=String(Math.min(requestedIndex,list.length-1));
  puzzleLabel.style.display='flex';
  const selected=list[Number(puzzleSelect.value)||0]||list[0];
  current=selected.board.map(row=>row.slice());givens=selected.board.map(row=>row.slice());
  solutionMode=false;renderBoard();resetStats();badgeEl.textContent=difficultySelect.value;
  setMessage(`${difficultySelect.value} puzzle loaded. Click Solve & Analyze.`);
}
function updateSize(){
  currentSize=Number(sizeSelect.value);[boxRows,boxCols]=sizes[currentSize];
  loadSelected();
}
function hasConflicts(){
  let found=false;const cells=[...boardEl.querySelectorAll('.cell')];cells.forEach(c=>c.classList.remove('invalid'));
  for(let r=0;r<currentSize;r++)for(let c=0;c<currentSize;c++){
    const v=current[r][c];if(!v)continue;let bad=false;
    for(let k=0;k<currentSize;k++)if((k!==c&&current[r][k]===v)||(k!==r&&current[k][c]===v))bad=true;
    const sr=Math.floor(r/boxRows)*boxRows,sc=Math.floor(c/boxCols)*boxCols;
    for(let rr=sr;rr<sr+boxRows;rr++)for(let cc=sc;cc<sc+boxCols;cc++)if((rr!==r||cc!==c)&&current[rr][cc]===v)bad=true;
    if(bad){cells[r*currentSize+c].classList.add('invalid');found=true;}
  }
  return found;
}
function showStats(data){
  document.getElementById('recursiveCalls').textContent=Number(data.stats.recursiveCalls).toLocaleString();
  document.getElementById('placements').textContent=Number(data.stats.attemptedPlacements).toLocaleString();
  document.getElementById('backtracks').textContent=Number(data.stats.backtracks).toLocaleString();
  const ms=Number(data.stats.elapsedMs);document.getElementById('elapsed').textContent=ms<1?ms.toFixed(3)+' ms':ms.toFixed(2)+' ms';
  const el=document.getElementById('solutionState');
  if(data.status==='unique'){el.className='solution-state';el.innerHTML='<span class="state-icon">✓</span><div><b>Unique solution found</b><small>Exactly one solution was found.</small></div>';}
  else if(data.status==='multiple'){el.className='solution-state warning';el.innerHTML='<span class="state-icon">≋</span><div><b>Multiple solutions</b><small>At least two solutions exist.</small></div>';}
  else{el.className='solution-state error';el.innerHTML='<span class="state-icon">!</span><div><b>No solution</b><small>This puzzle has no valid completion.</small></div>';}
}
async function solve(){
  if(hasConflicts()){badgeEl.textContent='Conflicts';setMessage('Repeated numbers found in a row, column, or box. Fix the highlighted cells first.',true);return;}
  solveBtn.disabled=true;solveBtn.textContent='Analyzing…';setMessage('Sending puzzle to the C++ solver…');
  try{
    const board=current.map(row=>row.map(v=>v?symbolFor(v):'0'));
    const res = await fetch('https://soduko-solver-e96i.onrender.com/api/solve', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({size:currentSize,boxRows,boxCols,board})});
    const data=await res.json();
    if(!data.ok){setMessage(data.message||'Could not process the puzzle.',true);badgeEl.textContent='Input error';return;}
    showStats(data);
    if(data.board&&data.status!=='unsolvable'){
      const original=givens.map(row=>row.slice());
      current=data.board.map(row=>row.map(valueFor));givens=original;renderBoard();
      showStats(data);badgeEl.textContent='Solved';
      setMessage(data.message+(data.status==='multiple'?' Showing one valid solution.':''));
    }else{badgeEl.textContent='Unsolvable';setMessage(data.message,true);}
   }catch(e){badgeEl.textContent='Backend offline';setMessage('Cannot connect to the online solver. Please try again shortly. '+e.message,true);}
  finally{solveBtn.disabled=false;solveBtn.textContent='Solve & Analyze';}
}
sizeSelect.addEventListener('change',updateSize);
difficultySelect.addEventListener('change',()=>{puzzleSelect.value='0';loadSelected();});
puzzleSelect.addEventListener('change',loadSelected);
loadBtn.addEventListener('click',loadSelected);
clearBtn.addEventListener('click',()=>{
  current=Array.from({length:currentSize},()=>Array(currentSize).fill(0));
  givens=Array.from({length:currentSize},()=>Array(currentSize).fill(0));
  difficultySelect.value='Custom';puzzleLabel.style.display='none';solutionMode=false;
  renderBoard();resetStats();badgeEl.textContent='Custom puzzle';setMessage('Board cleared. Enter your own puzzle.');
});
solveBtn.addEventListener('click',solve);
loadSelected();
