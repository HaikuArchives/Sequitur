#ifndef GLPUBLIC_GLUTILS_H
#define GLPUBLIC_GLUTILS_H

#include <GlPublic/GlDefs.h>
class GlImage;
class GlNode;

/***********************
 * IMAGES
 ***********************/
/* Utils for making the node images
 */
GlImage*		gl_new_node_image(GlNode* n, bool border = true);
/* Take ownership of the supplied image, and answer one with a
 * border depending on the type of IO.
 */
GlImage*		gl_node_image_border(GlImage* img, uint32 io);

/***********************
 * NOTIFICATION
 ***********************/
/* Find the message with the given code in the change message.
 */
status_t		gl_get_notifier_msg(const BMessage& changeMsg,
									uint32 code, BMessage& codeMsg);

#endif
