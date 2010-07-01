#ifndef _CHUNKATTR_H
#define _CHUNKATTR_H

/** 
 * @file chunkattributes.h
 *
 * @brief Chunk attribute structures.
 *
 */


/** 
Structure chunkAttrPrio is a basic selection of attributes a chunker could assign and schedulers could use to make better decisions.
 * More complex systems might need more attributes (e.g. scheduling deadline), these should use another specialized version of chunk attribute structure.
 * As an alternative, AVP (attribute value pairs) could have been used, but this seems too complex for our needs.
*/
typedef struct chunkAttrPrio{
 
        /**
         * An integer representing the class or group of this chunk.
         * used for example in the layered and MDC coding
         * (giuseppe tropea)
         */
        int category;

        /**
         * A double float representing the priority of this chunk.
         * used for example for a base layer in an SVC coding, or for audio chunks
         * (giuseppe tropea)
         */
        double priority;
} ChunkAttrPrio;

#endif


