const childProcess = require('child_process');

const { CHAPTEREXE_COMMAND, CHAPTEREXE_OUTPUT } = require("../settings");

exports.exec = filename => {
  return new Promise((resolve)=>{
    const args = ["-v", filename, "-s", "8", "-e", "4", "-o", CHAPTEREXE_OUTPUT];
    const child = childProcess.spawn(CHAPTEREXE_COMMAND, args);
    child.on('exit', (code)=>{
      resolve();
    });
    child.stderr.on('data', (data)=>{
      //console.error("chapter_exe " + data);
      let strbyline = String(data).split('\n');
      for (let i = 0; i < strbyline.length; i++) {
        if(strbyline[i] != ''){
          if(strbyline[i].startsWith('Creating')){
            console.error("AviSynth " + strbyline[i]);
          }else{
            console.error("chapter_exe " + strbyline[i]);
          }
        }else{
          console.error(strbyline[i]);
        }
      }
    });
  })
};
