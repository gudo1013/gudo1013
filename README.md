
Throughout my four years at St. Thomas, I have had the opportunity to work on and learn about various projects both inside computer science and outside in other fields. I have learned various programming languages, skills, and concepts that will be useful in the coming future. Here are some of those projects that have helped facilitate growth in my education.

Project 1: Pipeline Simulator - (README is not filled out)
 
The first project to highlight is from my Computer Architecture class. We were tasked with creating a pipelining simulator to mimic how a CPU may handle multiple instructions at a time. To contextualize this project, my partner Scott D. and I built this project after completing a multi-cycle CPU simulator as our previous project. With a basic understanding of how to simulate a CPU, the next task was to create a pipelining implementation. To explain what we had to accomplish, the idea of pipelining is to have a constant flow of instructions occuring at different stages. For example, in our implementation, each instruction had 5 stages that it had to go through: Instruction Fetch (IF), Instruction Decode (ID), Execute (EX), Memory (MEM), and Write Back (WB). Each cycle would move an instruction further down the stages until it all instructions were completed. Additionally, as one instruction moved forward, from IF to ID for example, another instruction would load in behind it, into IF in this case. While this is not very complicated to achieve on its own, the difficulty came when instructions began to affect other instructions. One instruction may change something in memory for an instruction immediately proceeding it, meaning that the pipeline must recognize when this can occur and inject empty instructions called NOOPs to stall the simulator in order for the memory to correctly affect the coming instruction. These types of problems were known as hazards, and each one called for a different solution such as port-forwarding or NOOP injection. This was definitely a worthwhile project to complete, but at the time was a major source of frustration and headaches. Each hazard that we needed to account for only presented more problems for our fixes for other hazards. While it was difficult to complete, it also was extremely helpful in understanding how a computer executes instructions.
