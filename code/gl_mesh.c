// gl_mesh.c: triangle model functions 

#include "quakedef.h"

/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/

model_t		*aliasmodel;
aliashdr_t	*paliashdr;

extern trivertx_t* poseverts[MAXALIASFRAMES];

/*
================
GL_MakeAliasModelDisplayLists
================
*/
void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr)
{
	m->dxrModel = GL_LoadDXRAliasMesh(m->name, m->numvertexes, poseverts[0], m->numTris, m->triangles, m->stverts);

	// Write Obj test
	//if (m->numTris > 300)
	//{
	//	FILE* f = fopen("test.obj", "wb");
	//
	//	for (int i = 0; i < m->numvertexes; i++) {
	//		fprintf(f, "v %d %d %d\n", poseverts[0][i].v[0], poseverts[0][i].v[1], poseverts[0][i].v[2]);
	//	}
	//
	//	for (int i = 0; i < m->numTris; i++) {
	//		int idx1 = m->triangles[i].vertindex[0];
	//		int idx2 = m->triangles[i].vertindex[1];
	//		int idx3 = m->triangles[i].vertindex[2];
	//
	//		fprintf(f, "f %d %d %d\n", idx1 + 1, idx2 + 1, idx3 + 1);
	//	}
	//
	//	fclose(f);
	//}
}

