/*
** Copyright (C) 2023 Rochus Keller (me@rochus-keller.ch) for LeanCreator
**
** This file is part of LeanCreator.
**
** $QT_BEGIN_LICENSE:LGPL21$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
*/

#include "Engine.h"
#include <QFile>
#include <QtDebug>
#include <stdarg.h>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <bslib.h>
#include <bsparser.h>
#include <bshost.h>
#include <bsrunner.h>
#include <bsvisitor.h>
}

using namespace busy;

class Engine::Imp
{
public:
    lua_State *L;
    BSLogger logger;
    void* loggerData;

    Imp():logger(0),loggerData(0){}
    bool ok() const { return L != 0; }

    void error(const char* file, int row, int col, const char* format, ... )
    {
        if( logger == 0 )
            return;
        BSRowCol loc;
        loc.row = row;
        loc.col = col;
        va_list args;
        va_start(args, format);
        logger(BS_Error, loggerData, file,loc, format, args );
        va_end(args);
    }
    bool call(int nargs = 0, int nres = 0, const char* what = 0)
    {
        const int err = lua_pcall(L,nargs,nres,0);
        switch( err )
        {
        case LUA_ERRRUN:
            {
                const char* msg = lua_tostring(L, -1 );
                if( logger == 0 && msg == 0 )
                    msg = "LUA_ERRRUN with no message";
                if( msg )
                {
                    if( logger )
                        error( what, 0, 0, msg);
                    else
                        qCritical() << msg;
                }
                lua_pop(L, 1 );  /* remove error message */
            }
            return false;
        case LUA_ERRMEM:
            qCritical() << "Lua memory exception";
            return false;
        case LUA_ERRERR:
            // should not happen
            qCritical() << "Lua unknown error";
            return false;
        }
        return true;
    }
};

static bool loadLib( lua_State *L, const QByteArray& source, const QByteArray& name)
{
    const int top = lua_gettop(L);

    const int status = luaL_loadbuffer( L, source, source.size(), name.constData() );
    switch( status )
    {
    case 0:
        // Stack: function
        break;
    case LUA_ERRSYNTAX:
    case LUA_ERRMEM:
        qCritical() << lua_tostring( L, -1 );
        lua_pop( L, 1 );  /* remove error message */
        // Stack: -
        return false;
    }

    const int err = lua_pcall( L, 0, 1, 0 );
    switch( err )
    {
    case LUA_ERRRUN:
        qCritical() << lua_tostring( L, -1 );
        lua_pop( L, 1 );  /* remove error message */
        return false;
    case LUA_ERRMEM:
        qCritical() << "Lua memory exception";
        return false;
    case LUA_ERRERR:
        // should not happen
        qCritical() << "Lua unknown error";
        return false;
    }

    // stack: lib
    lua_getfield(L, LUA_GLOBALSINDEX, "package"); // stack: lib, package
    lua_getfield(L, -1, "loaded"); // stack: lib, package, loaded
    lua_pushvalue(L, -3 ); // stack: lib, package, loaded, lib
    lua_setfield(L, -2, name.constData() ); // stack: lib, package, loaded,
    lua_pop(L,2); // stack: lib

    Q_ASSERT( top + 1 == lua_gettop(L) );
    return true;
}

Engine::Engine()
{
    d_imp = new Imp();
    d_imp->L = lua_open();
    if( d_imp->L == 0 )
        return;
    luaL_openlibs(d_imp->L);
    lua_pushcfunction(d_imp->L, bs_open_busy);
    lua_pushstring(d_imp->L, BS_BSLIBNAME);
    lua_call(d_imp->L, 1, 0);

    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveXref");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveNumRefs");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveLocInfo");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveFullAst");
    lua_pushboolean(d_imp->L,1);
    lua_setglobal(d_imp->L,"#haveXref");
}

Engine::~Engine()
{
    if( d_imp->L )
        lua_close(d_imp->L);
    delete d_imp;
}

void Engine::registerLogger(BSLogger l, void* data)
{
    if( d_imp->ok() )
    {
        d_imp->logger = l;
        d_imp->loggerData = data;
        bs_preset_logger(d_imp->L,l,data);
    }
}

bool Engine::parse(const ParseParams& params, bool checkTargets)
{

    lua_pushstring(d_imp->L,params.build_mode.constData());
    lua_setglobal(d_imp->L,"#build_mode");

    QFile script(":/busy/builtins.lua");
    if( !script.open(QIODevice::ReadOnly) )
    {
        qCritical() << "cannot open builtins.lua";
        return false;
    }
    const QByteArray source = script.readAll();

    if( !loadLib(d_imp->L,source,"builtins") )
        return false;
    const int builtins = lua_gettop(d_imp->L);
    lua_pushvalue(d_imp->L,builtins);
    lua_setglobal(d_imp->L,"#builtins");
    lua_getfield(d_imp->L,builtins,"#inst");
    lua_replace(d_imp->L,builtins);

    lua_pushstring(d_imp->L,params.cpu);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_cpu");
    lua_setfield(d_imp->L,builtins,"target_cpu");

    lua_pushstring(d_imp->L,params.os);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_os");
    lua_setfield(d_imp->L,builtins,"target_os");

    lua_pushstring(d_imp->L,params.wordsize);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_wordsize");
    lua_setfield(d_imp->L,builtins,"target_wordsize");

    lua_pushstring(d_imp->L,params.toolchain);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_toolchain");
    lua_setfield(d_imp->L,builtins,"target_toolchain");

    lua_pushnumber(d_imp->L,params.tcver);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_toolchain_ver");
    lua_setfield(d_imp->L,builtins,"target_toolchain_ver");

    if( bs_normalize_path2(params.toolchain_path) != BS_OK )
        d_imp->error(params.root_source_dir,0,0,"error normalizing toolchain path %s", params.toolchain_path.constData() );
    lua_pushstring(d_imp->L,bs_global_buffer());
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"#toolchain_path");
    lua_setfield(d_imp->L,builtins,"target_toolchain_path");

    lua_pushstring(d_imp->L,params.toolchain_prefix);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"#toolchain_prefix");
    lua_setfield(d_imp->L,builtins,"target_toolchain_prefix");

    lua_pushinteger(d_imp->L,0);
    lua_pushvalue(d_imp->L,-1);
    lua_setfield(d_imp->L,builtins,"host_cpu_ver");
    lua_setfield(d_imp->L,builtins,"target_cpu_ver");

    lua_pushcfunction(d_imp->L, bs_compile);
    lua_pushstring(d_imp->L,params.root_source_dir.constData());
    lua_pushstring(d_imp->L,params.root_build_dir.constData());
    if( params.params.isEmpty() )
        lua_pushnil(d_imp->L);
    else
    {
        lua_createtable(d_imp->L,0,params.params.size());
        const int table = lua_gettop(d_imp->L);
        for( int i = 0; i < params.params.size(); i++ )
        {
            lua_pushstring(d_imp->L,params.params[i].first.constData());
            if( params.params[i].second.isEmpty() )
                lua_pushstring(d_imp->L,"true");
            else
                lua_pushstring(d_imp->L,params.params[i].second.constData());
            lua_rawset(d_imp->L,table);
        }
    }
    bool res = true;
    if( d_imp->call(3,0,params.root_source_dir) )
    {
        if( checkTargets )
        {
            lua_pushcfunction(d_imp->L, bs_findProductsToProcess);
            lua_getglobal(d_imp->L,"#root");
            if( params.targets.isEmpty() )
                lua_pushnil(d_imp->L);
            else
            {
                lua_createtable(d_imp->L,0,params.targets.size());
                const int table = lua_gettop(d_imp->L);
                for( int i = 0; i < params.targets.size(); i++ )
                {
                    lua_pushstring(d_imp->L,params.targets[i].constData());
                    lua_pushboolean(d_imp->L,1);
                    lua_rawset(d_imp->L,table);
                }
            }
            lua_getmetatable(d_imp->L,builtins);
            if( d_imp->call(3,1,params.root_source_dir) )
            {
                const int array = lua_gettop(d_imp->L);
                lua_pushcfunction(d_imp->L, bs_markAllActive);
                lua_pushvalue(d_imp->L,array);
                lua_createtable(d_imp->L,0,0);
                res = d_imp->call(2,0,params.root_source_dir);
                lua_pop(d_imp->L,1); // array
            }else
                res = false;
        }
    }else
        res = false;
    lua_pop(d_imp->L,1); // builtins
    return res;
}

extern "C" {
    static int runcmd(const char* cmd, void* data)
    {
        QByteArrayList* list = (QByteArrayList*)data;
        list->append(cmd);
        return 0;
    }
}

QByteArrayList Engine::generateBuildCommands(const QByteArrayList& targets)
{
    QByteArrayList list;
    bs_preset_runcmd(d_imp->L,runcmd, &list);
    lua_pushcfunction(d_imp->L, bs_execute);
    lua_getglobal(d_imp->L,"#root");
    if( targets.isEmpty() )
        lua_pushnil(d_imp->L);
    else
    {
        lua_createtable(d_imp->L,0,targets.size());
        const int table = lua_gettop(d_imp->L);
        for( int i = 0; i < targets.size(); i++ )
        {
            lua_pushstring(d_imp->L,targets[i].constData());
            lua_pushstring(d_imp->L,"true");
            lua_rawset(d_imp->L,table);
        }
    }
    bool res = d_imp->call(2,0);
    return list;
}

bool Engine::createBuildDirs()
{
    if( !d_imp->ok() )
        return false;

    const int top = lua_gettop(d_imp->L);
    lua_pushcfunction(d_imp->L, bs_createBuildDirs);
    lua_getglobal(d_imp->L,"#root");
    lua_getglobal(d_imp->L,"#builtins");
    lua_getfield(d_imp->L,-1,"#inst");
    lua_replace(d_imp->L,-2);
    lua_getfield(d_imp->L,-1,"root_build_dir");
    lua_replace(d_imp->L,-2);
    lua_call(d_imp->L,2,0);
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return true;
}

bool Engine::visit(BSBeginOp b, BSOpParam p, BSEndOp e, BSForkGroup g, void* data, const QByteArrayList& targets)
{
    if( !d_imp->ok() )
        return false;

    const int top = lua_gettop(d_imp->L);

    // NOTE: no precheck required since operation is very fast

    lua_pushcfunction(d_imp->L, bs_resetOut );
    lua_getglobal(d_imp->L,"#root");
    bool res = d_imp->call(1,0);
    if( !res )
    {
        Q_ASSERT( top == lua_gettop(d_imp->L));
        return false;
    }

    lua_pushcfunction(d_imp->L, bs_findProductsToProcess);
    lua_getglobal(d_imp->L,"#root");
    if( targets.isEmpty() )
        lua_pushnil(d_imp->L);
    else
    {
        lua_createtable(d_imp->L,0,targets.size());
        const int table = lua_gettop(d_imp->L);
        for( int i = 0; i < targets.size(); i++ )
        {
            lua_pushstring(d_imp->L,targets[i].constData());
            lua_pushstring(d_imp->L,"true");
            lua_rawset(d_imp->L,table);
        }
    }
    lua_getglobal(d_imp->L,"#builtins");
    res = d_imp->call(3,1);
    if( !res )
    {
        Q_ASSERT( top == lua_gettop(d_imp->L));
        return false;
    }

    const int prods = lua_gettop(d_imp->L);

    for( int i = 1; i <= lua_objlen(d_imp->L,prods); i++ )
    {
        lua_pushcfunction(d_imp->L, bs_visit);
        lua_rawgeti(d_imp->L,prods,i);
        BSVisitorCtx* ctx = bs_newctx(d_imp->L);
        ctx->d_data = data;
        ctx->d_begin = b;
        ctx->d_end = e;
        ctx->d_param = p;
        ctx->d_fork = g;
        ctx->d_log = d_imp->logger;
        res = d_imp->call(2,0);
        if( !res )
        {
            lua_pop(d_imp->L, 1); // prods
            Q_ASSERT( top == lua_gettop(d_imp->L));
            return false;
        }
    }

    lua_pop(d_imp->L, 1); // prods
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return true;
}

int Engine::getRootModule() const
{
    if( d_imp->ok() )
    {
        const int top = lua_gettop(d_imp->L);

        lua_getglobal(d_imp->L,"#root");
        if( lua_istable(d_imp->L,-1) )
        {
            lua_getfield(d_imp->L,-1,"#ref");
            const int ref = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,2);
            return ref;
        }else
            lua_pop(d_imp->L,1);

        Q_ASSERT( top == lua_gettop(d_imp->L) );
    }
    return 0;
}

int Engine::findModule(const QString& path) const
{
    int res = 0;
    lua_getglobal(d_imp->L,"#refs");
    if( lua_istable(d_imp->L,-1) )
    {
        lua_pushstring(d_imp->L,path.toUtf8().constData());
        lua_rawget(d_imp->L,-2);
        if( lua_istable(d_imp->L,-1) )
        {
            lua_getfield(d_imp->L,-1,"#ref");
            res = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    return res;
}

QList<int> Engine::findDeclByPos(const QString& path, int row, int col) const
{
    QList<int> res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    lua_getglobal(d_imp->L,"#xref");
    if( lua_istable(d_imp->L,-1) )
    {
        // filepath -> list_of_idents{ rowcol -> set_of_decls }
        lua_pushstring(d_imp->L,path.toUtf8().constData());
        lua_rawget(d_imp->L,-2);
        if( lua_istable(d_imp->L,-1) )
        {
            const int list_of_idents = lua_gettop(d_imp->L);
            lua_pushinteger(d_imp->L, bs_torowcol(row,col) );
            lua_rawget(d_imp->L,list_of_idents);
            if( lua_istable(d_imp->L,-1) )
            {
                const int set_of_decls = lua_gettop(d_imp->L);
                lua_pushnil(d_imp->L);  /* first key */
                while (lua_next(d_imp->L, set_of_decls) != 0)
                {
                    const int key = lua_gettop(d_imp->L)-1;
                    const int ref = assureRef(key);
                    if( ref )
                        res << ref;
                    lua_pop(d_imp->L,1);
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QString Engine::findPathByPos(const QString& path, int row, int col) const
{
    QString res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    lua_getglobal(d_imp->L,"#xref");
    if( lua_istable(d_imp->L,-1) )
    {
        // filepath -> list_of_idents{ rowcol -> path }
        lua_pushstring(d_imp->L,path.toUtf8().constData());
        lua_rawget(d_imp->L,-2);
        if( lua_istable(d_imp->L,-1) )
        {
            const int list_of_idents = lua_gettop(d_imp->L);
            lua_pushinteger(d_imp->L, bs_torowcol(row,col) );
            lua_rawget(d_imp->L,list_of_idents);
            if( lua_isstring(d_imp->L,-1) )
            {
                res = QString::fromUtf8(bs_denormalize_path(lua_tostring(d_imp->L,-1)));
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<Engine::AllLocsInFile> Engine::findAllLocsOf(int id) const
{
    QList<Engine::AllLocsInFile> res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(id) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#name");
        const QString name = QString::fromUtf8(lua_tostring(d_imp->L,-1));
        lua_pop(d_imp->L,1);
        lua_getfield(d_imp->L,decl,"#xref");
        const int refs = lua_gettop(d_imp->L);
        if( lua_istable(d_imp->L,refs) )
        {
            lua_pushnil(d_imp->L);  /* first key */
            while (lua_next(d_imp->L, refs) != 0)
            {
                AllLocsInFile a;
                a.d_file = QString::fromUtf8(lua_tostring(d_imp->L,-2));
                const int set_of_rowcol = lua_gettop(d_imp->L);
                lua_pushnil(d_imp->L);  /* first key */
                while (lua_next(d_imp->L, set_of_rowcol) != 0)
                {
                    const int rowCol = lua_tointeger(d_imp->L,-2);
                    Loc l;
                    l.d_row = bs_torow(rowCol);
                    l.d_col = bs_tocol(rowCol) + 1;
                    l.d_len = name.size();
                    a.d_locs.append(l);
                    lua_pop(d_imp->L, 1);
                }
                lua_pop(d_imp->L, 1);
                res.append(a);
            }
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<Engine::Loc> Engine::findDeclInstsInFile(const QString& path, int id) const
{
    QList<Engine::Loc> res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(id) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#name");
        const QString name = QString::fromUtf8(lua_tostring(d_imp->L,-1));
        lua_pop(d_imp->L,1);
        lua_getfield(d_imp->L,decl,"#xref");
        const int refs = lua_gettop(d_imp->L);
        if( lua_istable(d_imp->L,refs) )
        {
            lua_pushstring(d_imp->L,path.toUtf8().constData());
            lua_rawget(d_imp->L,refs);
            const int set_of_rowcol = lua_gettop(d_imp->L);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_pushnil(d_imp->L);  /* first key */
                while (lua_next(d_imp->L, set_of_rowcol) != 0)
                {
                    const int key = lua_gettop(d_imp->L)-1;
                    const int rowCol = lua_tointeger(d_imp->L,key);
                    Loc loc;
                    loc.d_row = bs_torow(rowCol);
                    loc.d_col = bs_tocol(rowCol)+1;
                    loc.d_len = name.size();
                    res << loc;
                    lua_pop(d_imp->L, 1);
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<int> Engine::getSubModules(int id) const
{
    QList<int> res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(id) )
    {
        const int n = lua_objlen(d_imp->L,-1);
        for( int i = 1; i <= n; i++ )
        {
            lua_rawgeti(d_imp->L,-1,i);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_getfield(d_imp->L,-1,"#kind");
                const int k = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                if( k == BS_ModuleDef )
                {
                    lua_getfield(d_imp->L,-1,"#ref");
                    const int ref = lua_tointeger(d_imp->L,-1);
                    lua_pop(d_imp->L,1);
                    res += ref;
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<int> Engine::getAllProducts(int id, ProductFilter filter, bool onlyActives) const
{
    QList<int> res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(id) )
    {
        const int inst = lua_gettop(d_imp->L);
        lua_getglobal(d_imp->L,"#builtins");

        if( filter == Executable )
            lua_getfield(d_imp->L,-1,"Executable");
        else if( filter == Compiled )
            lua_getfield(d_imp->L,-1,"CompiledProduct");
        else
            lua_getfield(d_imp->L,-1,"Product");
        lua_replace(d_imp->L,-2);
        const int productClass = lua_gettop(d_imp->L);

        const int n = lua_objlen(d_imp->L,inst);
        for( int i = 1; i <= n; i++ )
        {
            lua_rawgeti(d_imp->L,inst,i);
            const int decl = lua_gettop(d_imp->L);
            if( lua_istable(d_imp->L,decl) )
            {
                lua_getfield(d_imp->L,decl,"#kind");
                const int k = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                lua_getfield(d_imp->L,decl,"#ctr");
                const int hasBody = !lua_isnil(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                lua_getfield(d_imp->L,decl,"#active");
                const int isActive = !lua_isnil(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                if( k == BS_VarDecl && hasBody && ( isActive || !onlyActives ) )
                {
                    lua_getfield(d_imp->L,-1,"#type");
                    Q_ASSERT( !lua_isnil(d_imp->L,-1) );
                    const bool isProduct = bs_isa(d_imp->L,productClass,-1);
                    bool hasSource = false;
                    if( filter == WithSources )
                    {
                        lua_getfield(d_imp->L,-1,"sources");
                        lua_getfield(d_imp->L,-2,"use_deps"); // exclude Copy
                        hasSource = !lua_isnil(d_imp->L,-2) && lua_isnil(d_imp->L,-1);
                        lua_pop(d_imp->L,2);
                    }
                    lua_pop(d_imp->L,1);
                    if( isProduct || hasSource )
                    {
                        lua_getfield(d_imp->L,-1,"#ref");
                        const int ref = lua_tointeger(d_imp->L,-1);
                        lua_pop(d_imp->L,1);
                        res += ref;
                    }
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,2);
    }
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

QList<int> Engine::getAllDecls(int module) const
{
    QList<int> res;
    if( d_imp->ok() && pushInst(module) )
    {
        const int decl = lua_gettop(d_imp->L);
        const int n = lua_objlen(d_imp->L,decl);
        for( int i = 1; i <= n; i++ )
        {
            lua_rawgeti(d_imp->L,decl,i);
            if( lua_istable(d_imp->L,-1) )
            {
                lua_getfield(d_imp->L,-1,"#kind");
                const int k = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                if( k == BS_ModuleDef || k == BS_ClassDecl || k == BS_EnumDecl ||
                        k == BS_VarDecl || k == BS_FieldDecl || k == BS_MacroDef )
                {
                    const int id = assureRef(-1);
                    if( id )
                        res.append(id);
                }
            }
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,1);
    }
    return res;
}

QStringList Engine::getAllSources(int product, bool addGenerated) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);

        lua_getfield(d_imp->L,decl,"#owner");
        lua_getfield(d_imp->L,-1,"#dir");
        lua_replace(d_imp->L,-2);
        const int absDir = lua_gettop(d_imp->L);

        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,inst,"sources");
        const int sources = lua_gettop(d_imp->L);
        if( !lua_isnil(d_imp->L,sources) )
        {
            const int n = lua_objlen(d_imp->L,sources);
            for( int i = 1; i <= n; i++ )
            {
                lua_rawgeti(d_imp->L,sources,i);
                const int file = lua_gettop(d_imp->L);
                if( *lua_tostring(d_imp->L,file) != '/' )
                {
                    if( bs_add_path(d_imp->L,absDir,file) == 0 )
                        lua_replace(d_imp->L,file);
                }
                res << QString::fromUtf8( bs_denormalize_path(lua_tostring(d_imp->L,file)) );

                lua_pop(d_imp->L,1);
            }
        }

        if( addGenerated )
        {
            lua_getfield(d_imp->L,inst,"#generated");
            const int generated = lua_gettop(d_imp->L);
            const int n = lua_objlen(d_imp->L,generated);
            for( int i = 1; i <= n; i++ )
            {
                lua_rawgeti(d_imp->L,generated,i);
                const int file = lua_gettop(d_imp->L);
                if( *lua_tostring(d_imp->L,file) != '/' )
                {
                    if( bs_add_path(d_imp->L,absDir,file) == 0 )
                        lua_replace(d_imp->L,file);
                }
                res << QString::fromUtf8( bs_denormalize_path(lua_tostring(d_imp->L,file)) );

                lua_pop(d_imp->L,1);
            }
            lua_pop(d_imp->L,1); // generated
        }
        lua_pop(d_imp->L,4);
    }
    return res;
}

static void fetchConfig(lua_State* L,int inst, const char* field, QStringList& result, bool isPath)
{
    lua_getfield(L,inst,"configs");
    const int configs = lua_gettop(L);
    size_t i;
    for( i = 1; i <= lua_objlen(L,configs); i++ )
    {
        lua_rawgeti(L,configs,i);
        const int config = lua_gettop(L);
        // TODO: check for circular deps
        fetchConfig(L,config,field,result, isPath);
        lua_pop(L,1); // config
    }
    lua_pop(L,1); // configs

    bs_getModuleVar(L,inst,"#dir");
    const int absDir = lua_gettop(L);

    lua_getfield(L,inst,field);
    const int list = lua_gettop(L);

    for( i = 1; i <= lua_objlen(L,list); i++ )
    {
        lua_rawgeti(L,list,i);
        const int item = lua_gettop(L);
        if( isPath && *lua_tostring(L,-1) != '/' )
        {
            // relative path
            if( bs_add_path(L,absDir,item) == 0 )
                lua_replace(L,item);
        }
        if( isPath )
            result << QString::fromUtf8( bs_denormalize_path(lua_tostring(L,item)) );
        else
            result << QString::fromUtf8( lua_tostring(L,item) );
        lua_pop(L,1); // path
    }
    lua_pop(L,2); // absDir, list
}

QStringList Engine::getIncludePaths(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"include_dirs",res,true);
        lua_pop(d_imp->L,2);
    }
    return res;
}

QStringList Engine::getDefines(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"defines",res,false);
        lua_pop(d_imp->L,2);
    }
    return res;
}

QStringList Engine::getCppFlags(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"cflags",res,false);
        fetchConfig(d_imp->L,inst,"cflags_cc",res,false);
        lua_pop(d_imp->L,2);
    }
    return res;
}

QStringList Engine::getCFlags(int product) const
{
    QStringList res;
    if( d_imp->ok() && pushInst(product) )
    {
        const int decl = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,decl,"#inst");
        const int inst = lua_gettop(d_imp->L);
        fetchConfig(d_imp->L,inst,"cflags",res,false);
        fetchConfig(d_imp->L,inst,"cflags_c",res,false);
        lua_pop(d_imp->L,2);
    }
    return res;
}

bool Engine::isClass(int id, const char *clsName) const
{
    if( !d_imp->ok() )
        return false;
    const int top = lua_gettop(d_imp->L);
    bool isClass = false;
    if( pushInst(id) )
    {
        const int inst = lua_gettop(d_imp->L);
        lua_getglobal(d_imp->L,"#builtins");
        lua_getfield(d_imp->L,-1,clsName);
        const int cls = lua_gettop(d_imp->L);

        lua_getfield(d_imp->L,inst,"#kind");
        const int k = lua_tointeger(d_imp->L,-1);
        lua_pop(d_imp->L,1); // kind
        if( k == BS_VarDecl )
        {
            lua_getfield(d_imp->L,inst,"#type");
            isClass = bs_isa(d_imp->L,cls,-1);            
            lua_pop(d_imp->L,1); // type
        }
        lua_pop(d_imp->L,3); // inst, builtins, cls
    }
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return isClass;
}

bool Engine::isExecutable(int id) const
{
    return isClass(id, "Executable");
}

bool Engine::isActive(int id) const
{
    if( !d_imp->ok() )
        return false;
    if( pushInst(id) )
    {
        const int inst = lua_gettop(d_imp->L);
        lua_getfield(d_imp->L,inst,"#active");
        const bool res = !lua_isnil(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return res;
    }else
        return false;
}

QByteArray Engine::getString(int def, const char* field, bool inst) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        if( inst )
        {
            lua_getfield(d_imp->L,-1,"#inst");
            if( !lua_isnil(d_imp->L,-1) )
                lua_replace(d_imp->L,-2);
            else
                lua_pop(d_imp->L,1);
        }
        lua_getfield(d_imp->L,-1,field);
        const QByteArray res = lua_tostring(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return res;
    }
    return QByteArray();
}

QByteArray Engine::getDeclPath(int decl) const
{
    QByteArray res;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(decl) )
    {
        bs_declpath(d_imp->L,-1,".");
        res = lua_tostring(d_imp->L,-1);
        lua_pop(d_imp->L,2); // inst, declpath
    }
    Q_ASSERT( top == lua_gettop(d_imp->L) );
    return res;
}

int Engine::getInteger(int def, const char* field) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,field);
        const int res = lua_tointeger(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return res;
    }
    return 0;
}

QString Engine::getPath(int def, const char* field) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,field);
        const QByteArray res = lua_tostring(d_imp->L,-1);
        lua_pop(d_imp->L,2);
        return QString::fromUtf8(bs_denormalize_path(res.constData()));
    }
    return QString();
}

int Engine::getObject(int def, const char* field) const
{
    int res = 0;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,field);
        if( lua_istable(d_imp->L,-1) )
        {
            const int table = lua_gettop(d_imp->L);
            res = assureRef(table);
        }
        lua_pop(d_imp->L,2);
    }
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return res;
}

int Engine::getGlobals() const
{
    int res = 0;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    lua_getglobal(d_imp->L,"#builtins");
    lua_getfield(d_imp->L,-1,"#inst");
    res = assureRef(-1);
    lua_pop(d_imp->L,2);
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return res;
}

int Engine::getOwningModule(int def) const
{
    int res = 0;
    if( !d_imp->ok() )
        return res;
    const int top = lua_gettop(d_imp->L);
    if( pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,"#owner");
        const int table = lua_gettop(d_imp->L);
        while( !lua_isnil(d_imp->L,table) )
        {
            lua_getfield(d_imp->L,table,"#kind");
            const int k = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,1);
            if( k == BS_ModuleDef )
            {
                lua_getfield(d_imp->L,table,"#ref");
                res = lua_tointeger(d_imp->L,-1);
                lua_pop(d_imp->L,1);
                lua_pushnil(d_imp->L);
                lua_replace(d_imp->L,table);
            }else
            {
                lua_getfield(d_imp->L,table,"#owner");
                lua_replace(d_imp->L,table);
            }
        }
        lua_pop(d_imp->L,1);
    }
    lua_pop(d_imp->L,1);
    Q_ASSERT( top == lua_gettop(d_imp->L));
    return res;
}

int Engine::getOwner(int def) const
{
    int res = 0;
    if( d_imp->ok() && pushInst(def) )
    {
        lua_getfield(d_imp->L,-1,"#owner");
        if( lua_istable(d_imp->L,-1) )
        {
            lua_getfield(d_imp->L,-1,"#ref");
            res = lua_tointeger(d_imp->L,-1);
            lua_pop(d_imp->L,1);
        }
        lua_pop(d_imp->L,2);
    }
    return res;
}

void Engine::dump(int def, const char* title) const
{
    if( d_imp->ok() && pushInst(def) )
    {
        bs_dump2(d_imp->L,title,-1);
    }
}

bool Engine::build(const QByteArrayList& targets, BSRunCmd runcmd, void* data)
{
    bs_preset_runcmd(d_imp->L,runcmd, data);
    lua_pushcfunction(d_imp->L, bs_execute);
    lua_getglobal(d_imp->L,"#root");
    if( targets.isEmpty() )
        lua_pushnil(d_imp->L);
    else
    {
        lua_createtable(d_imp->L,0,targets.size());
        const int table = lua_gettop(d_imp->L);
        for( int i = 0; i < targets.size(); i++ )
        {
            lua_pushstring(d_imp->L,targets[i].constData());
            lua_pushstring(d_imp->L,"true");
            lua_rawset(d_imp->L,table);
        }
    }
    return d_imp->call(2,0);
}

bool Engine::pushInst(int ref) const
{
    int n = 1;
    lua_getglobal(d_imp->L,"#refs");
    if( lua_istable(d_imp->L,-1) )
    {
        lua_rawgeti(d_imp->L,-1,ref);
        n++;
        if( lua_istable(d_imp->L,-1) )
        {
            lua_replace(d_imp->L,-2);
            return true;
        }
    }
    lua_pop(d_imp->L,n);
    return false;
}

int Engine::assureRef(int table) const
{
    int res = 0;
    const int top = lua_gettop(d_imp->L);
    if( table <= 0 )
        table += top + 1;
    lua_getfield(d_imp->L,table,"#ref");
    if( lua_isnil(d_imp->L,-1) )
    {
        // the object doesn't have a ref, so create and register one
        lua_getglobal(d_imp->L,"#refs");
        const int refs = lua_gettop(d_imp->L);
        res = lua_objlen(d_imp->L,refs) + 1;
        lua_pushinteger(d_imp->L,res);
        lua_setfield(d_imp->L,table,"#ref");
        lua_pushvalue(d_imp->L,table);
        lua_rawseti(d_imp->L,refs,res);
        lua_pop(d_imp->L,1);
    }else
        res = lua_tointeger(d_imp->L,-1);
    lua_pop(d_imp->L,1);
    return res;
}

