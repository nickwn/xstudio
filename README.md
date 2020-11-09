# xstudio

graphics engine type of thing

Features:
 * c++17, minimal deps, etc
 * entt runtime-editable evaluation graph
 * vulkan backend, simple kinda stateless renderer

Eval graph is still a wip but right now I organized things into:
 * data source nodes, which output data but do no evaluation on their own
 * eval nodes, which perform operations on input nodes and potentially output data
 * applicators, which apply evaluated output data from a node to the scene

I'm using a particle system as an example use of the eval graph, but it is still in progress.
Uses an SPH algorithm with a spatial hash table for neighbor lookup which stores: 
 * a sorted array of all particles based on their morton code
 * a hash table with the key being a coordinate hash and the value being an index into the sorted array 

A few of the many issues to be worked through:
 * every eval node stores the index of the particle it is working on
 * no eval synchronization barriers
 * bad math library