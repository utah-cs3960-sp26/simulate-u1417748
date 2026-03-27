# Week 10 


## Look through the history of all edits you ever made to your prompt. Detail the changes you made and why you made them. What Amp misbehaviors did you notice? What and why did you add to the prompt file? Did you remove anything?


I did not need to make any edits to my prompt and it was able to successfully loop through and complete it when it was "done" looping. Some things to note are that I used an LLM to help me detail and write a complete prompt that would cover a lot more edge cases and give AMP better directions and edge cases to handle. For example, incorporating parts of the reading, I gave AMP some instructions for how to test their implementation so that it wouldn't have to guess everytime if it works and instead could run the tests itself and see if the program is working as expected. I did however notice that I needed to give AMP further permissions in order to be able to allow it to run the test files as default, it wasn't allowed to run tools with Bash commands. Overall, through going back and forth with an LLM to find initial ways to give a more guiding initial prompt, AMP was able to successfully get the physics environment working in a single loop. 


# Week 11

## Detail the changes you made and why you made them. What Amp misbehaviors did you notice? What and why did you add to the prompt file? Did you remove anything?

I made it so that you can upload a CSV to start from and you can draw onto the dots at the end before saving the CSV so that you can:

1) run the simulation
2) draw on the dots at the final position 
3) re-load the app using that CSV
4) watch the dots land into their correct positions

Also, the original prompts did not have a "saved" velocity which caused the balls to fall randomly every time and not in the same spot. I did not have to remove anything, but had to do a lot of back and forth with AMP to discover bug causes and to add some cool features like painting to help with testing the physics and consistency.  

Overall, I got a working physics simulator that can consistently re-run the ball path and location to animate a drawn design.