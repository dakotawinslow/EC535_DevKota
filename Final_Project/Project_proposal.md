# EC535 Project Proposal

## Controllable Limo Swarm

Self-organizing group of robots (minimum 2) that can be commanded as a group.

### Basic Requirements:

- Remote Control via PS4 Controller or similar
    
- “Group-based” control, where control inputs are sent to the swarm as a whole and interpreted as a group (may still be leader-based, but invisibly so)
    
- Must use some sensing to maintain formation spacing/regularity
    
- Basic formation is a grid

### Additional Goals:

- Extra Formations:
	- Column: a high-aspect ratio “marching line” which is controlled from the front and moves like a snake, maintaining close spacing between bots and keeping the line (no drift)
	- Circle of protection: Continuously circle the control point at variable speed and radius.
	- Escort: Surround the control point in a moving formation for protection while moving. Dynamically optimize shape based on number of bots.
- Voice Control:
	- Change formation and parameters based on voice commands, possibly processed off-bot by a remote server system.
- Person Tracking: 
	- Make the control point a person so the formation follows them.

### Minimum requirements for success:
- 2 Limo robots moving in response to controller inputs.
- Data Visualization tool showing the control point and the robots' spatial relationship to it.
### Challenges
- LIMO Robots are actually quite old and out of date, many dependencies are breaking.
- Ideal movement requires Mechanum wheels (omni-wheels) but few robots still seem to have them.
- ROS is almost certainly going to be required to make this work, but using it can be difficult.
- We have limited time and heavy workloads, including other projects and a mysterious exam for THIS CLASS.

## Work Chains
These are threads of work that need to be done to achieve success that, in the beginning, can be worked on separately.

Motion Platform setup - moving single robot via DS5 Controller

Data Visualizer - Find/Create software to show digital representation of robots in space along with directed area

Sensing - Select and develop inter-robot sensing system (LiDAR, RASTIC MoCap, Depth Camera, or other)

