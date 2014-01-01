#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlRootNode.h>
#include <GlPublic/GlRootRef.h>
#include <GlKernel/GlDefs.h>
#include <Glasslike/GlDefs.h>

const char*			GL_NODE_ADDON_ID_STR			= "na_id";
const char*			GL_INSPECTOR_ON_STR				= "ion";
const char*			GL_INSPECTOR_TOP_STR			= "it";
const char*			GL_INSPECTOR_WIDTH_STR			= "iw";
const char*			GL_NODE_VIEW_STR				= "nv";
const char*			GL_I_STR						= "i";
const char*			GL_ROOT_STR						= "root";
const char*			GL_TEXT_STR						= "text";
const char*			GL_UNTITLED_STR					= "Untitled";

const uint32		GL_ROOT_MSG						= 'root';

GlRootNode* gl_new_root_node(int32 type)
{
	const GlNodeAddOn*	addon = GlGlobals().GetAddOn(GL_ROOT_KEY);
	if (!addon) return 0;
	return new GlRootNode(addon, type);
}

void gl_get_root_label(const GlRootRef& ref, BString16& outLabel)
{
	const GlRootNode*		root = ref.ReadLock();
	if (root) {
		gl_get_root_label(root, outLabel);
		ref.ReadUnlock(root);
	}
	if (outLabel.Length() < 1) outLabel = GL_UNTITLED_STR;
}

void gl_get_root_label(const GlRootNode* root, BString16& outLabel)
{
	if (root) outLabel = root->Label();
	if (outLabel.Length() < 1) outLabel = GL_UNTITLED_STR;
}
