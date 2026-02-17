add the go implementation.  
- it is important that the code have the same functionality but it doesn't have to be strictly modeled after the rust implementation
- it should have a similar structure with a main function that gathers command line arguments and  a functiuon that executes the query.
- it should be idiomatic go code. 
- use standard libraries if possible. 
- use the superpower skill if that makes sense   
- include tests     

add the python implementation.  
- it is important that the code have the same functionality but it doesn't have to be strictly modeled after the rust implementation
- it should have a similar structure with a main function that gathers command line arguments and  a functiuon that executes the query.
- it should be idiomatic python code. 
- use standard libraries if possible. 
- use the superpower skill if that makes sense   
- include tests but keep them simple. don't use an elaborate test harness     

add the typescript implementation.  
- it is important that the code have the same functionality but it doesn't have to be strictly modeled after the rust implementation
- it should have a similar structure with a main function that gathers command line arguments and  a functiuon that executes the query.
- it should be idiomatic typescript code. 
- use standard libraries if possible. 
- use the superpower skill if that makes sense   
- include tests but keep them simple. don't use an elaborate test harness     
- for run.sh, use TSX: Modern, esbuild‑based runner; much faster than ts-node and handles ESM/CJS smoothly.
```
npm install -D tsx.
Run: npx tsx src/main.ts.
```