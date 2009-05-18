
Someone asked the following question about implementing LN on other processors:

"Assuming that 10% of the Nucleus code is ipc code, what would be the overall
performance if on PPC these 10% would be implemented highly optimized in assembler
while the other 90% would be implemented in a higher-level language performing
only half as fast?"

Although the question seems simple, there are two problems with it: the presupposition
"10% ipc code" is wrong (a), and the question itself may be a red herring (b).
I think the real question is "What performance can we achieve with what implementation
methodology / implementation costs (multiple scenarios)?" (Conclusion)



a)      The assumption that only 10% of the Nucleus code are ipc is wrong. In fact,
        about 40% of the (non-initialization) code is ipc. If mapping (which is part
        of ipc) is added, even 50% to 60% belong to ipc. Obviously, a 50:50 ass:C model
        is much less attractive than the assumed 10:90 model, in particular, since
        the non-ipc 50% have much simpler logic than the ipc routines.
        
        However, when aiming at a first LN implementation being 1.3 to 2 times slower
        than the optimum, probably only 10% to 20% of the ipc code had to bee highly
        optimized, i.e. only the original 10% of the entire LN code. 
        
        However, optimizing only the critical path of ipc might be a big implementation 
        problem. Since "the critical path" is not a logical entity like a module or a 
        procedure but a set of basic blocks spread among multiple procedures and even
        modules, glueing the assembler and the higher-level-language parts together 
        might become difficult and/or expensive. The costs depend heavily on language 
        and its compiler, in particular on the code generator and the link conventions. 
        I think that there is a good chance to master the problems. However, for a sound
        answer, we have to do a basic design (on paper) of the ipc part, using a concrete
        processor and a conrete compiler/code generator.
        
        
b)      The original question seems to assume that the LN performance is basically due
        to the higly micro-optimized code (doing code generation by hand better than
        a compiler). When you look at the 93 SOSP paper, you see that coding-level
        optimizations are only responsible for the last factor of 2 to 3. Relate this
        to the overall factor of 20 to 50 (LN ipc being faster than some other ipc).
        This illustrates the importance and effect of conceptual and architectural 
        optimizations. Although important at the very end, coding-level optimizations
        make sense only if the mentioned conceptual and architectural optimizations
        could be fully implemented and utilized.
        
        This however, is not trivial. It requires a Nucleus design integrated with
        the hardware architecture (see also 95 SOSP paper). Such a design necessarily
        works on the machine-architecture level (what registers can be used when,
        how entering kernel mode for system call / faults / device interrupts, how
        to handle TLB misses, using physical or virtual addresses for tcbs, etc.) 
        The critical question is whether and how the implementation methodolgy
        (language, compiler, code generator) permits to implement the basic design.
        If we get problems on this level, they might impose even worse performance
        effects than the coding-level problems discussed in (a).
        Once more, I am optimistic, but a basic LN design based on a concrete
        implementation tool is required to get sound answers.
        
        

Conclusion
        
"What performance can we achieve with what implementation methodology / implementation costs?"         

That is probably the basic question we are interested in for any LN implementation. As (a)
and (b) already elucidated, answering the question requires detailed analysis of both the
target hardware architecture and the available implementation tools. For my experience,
the best way is to make a basic "first-cut" design on paper by three experts: a Nucleus
expert, a target-hardware expert, and a compiler expert:

  1.    Design the critical ipc path and the TLB-miss handling. This covers most 
        performance-critical parts as well as most basic architectural design decisions:
        how to enter kernel mode, how to address tcbs, interface conventions, basic
        address-space structure, page-table structure ...
        
        Since this task is strongly influenced by the machine architecture (registers, exceptions,
        processor modes, TLBs, caches, instruction timing) and the algorithmic logic is rather
        simple, the design should be based on machine instructions complemented by usual 
        data structures and pseudo code. Using C at this level instead of machine instructions
        would make things much more difficult because it would not simplify the real problem
        (which is the machine architecture) and additionally introduce the code-generator
        problem.
        
        The outcome of this step is twofold: (a) the basic architectural design decisions
        as mentioned above, and (b) a good estimation for the optimally achievable LN performance
        on this architecture. (b) is based on a timing analysis of the resulting critical
        ipc code and the TLB miss handler. For my experience, the first version of a real
        implementation (fully optimized) can be expected with +-20% of this estimation.
        
        
  2.    In a second step, the design of (1) should be partially redone based on a (or some)
        higher-level language tool(s), e.g. a C compiler. At least two scenarios should be analyzed:
              2a)   complete C implementation (rewrite all of step 1 in C) ;
              2b)   partial C implementation (glue all machine code of step 1 to some C skeleton).
        For both cases, structural consequences and performance implications (based on a thorough
        inspection of the generated code) must be carefully analyzed.
        
        
Combining the outputs of both steps, we will have substantiated performance and implemetation-costs
estimations for various implementation models. Three weeks should be sufficient for both steps
provided that the experts are experts, the tools are available, and the work is not interrupted 
by other activities. 

        Jochen Liedtke, 01/13/98
        
        
        
        
        
        
        


























