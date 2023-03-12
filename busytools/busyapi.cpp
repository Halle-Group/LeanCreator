/*
** Copyright (C) 2022 Rochus Keller (me@rochus-keller.ch) for LeanCreator
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

#include "busyapi.h"
#include "Engine.h"
#include "busyBuilder.h"
extern "C" {
#include <bshost.h>
#include <bsparser.h>
}
#include <QFileInfo>
#include <QDir>
#include <QtDebug>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
using namespace busy;

class Internal::ModuleImp : public QSharedData
{
public:
    Engine::Ptr d_eng;
    int d_id;
};

Module::Module(Engine* eng, int id)
{
    d_imp = new Internal::ModuleImp();
    d_imp->d_eng = eng;
    d_imp->d_id = id;
}

Module::Module()
{

}

Module::Module(const Module& other) : d_imp(other.d_imp)
{

}

Module&Module::operator=(const Module& other)
{
    d_imp = other.d_imp;
    return *this;
}

Module::~Module()
{

}

bool Module::isValid() const
{
    return d_imp.data() != 0;
}

QString Module::name() const
{
    if( !isValid() )
        return QString();
    return QString::fromUtf8( d_imp->d_eng->getString(d_imp->d_id,"#name") );
}

CodeLocation Module::location() const
{
    CodeLocation res;
    if( !isValid() )
        return res;
    const int owner = d_imp->d_eng->getOwner(d_imp->d_id);
    const QByteArray tmp = d_imp->d_eng->getString(owner ? owner : d_imp->d_id,"#file");
    res.d_path = QString::fromUtf8( bs_denormalize_path(tmp.constData()) );
    if( owner )
    {
        res.d_col = d_imp->d_eng->getInteger(d_imp->d_id,"#col") + 1;
        res.d_row = d_imp->d_eng->getInteger(d_imp->d_id,"#row");
    }
    return res;
}

QString Module::busyFile() const
{
    QString res;
    if( !isValid() )
        return res;
    const QByteArray tmp = d_imp->d_eng->getString(d_imp->d_id,"#file");
    res = QString::fromUtf8( bs_denormalize_path(tmp.constData()) );
    return res;
}

QString Module::buildDirectory() const
{
    return QString();
}

QList<Product> Module::products(bool withSourcesOnly) const
{
    QList<Product> res;
    if( !isValid() )
        return res;
    QList<int> ids = d_imp->d_eng->getAllProducts(d_imp->d_id,withSourcesOnly ?
                                                      Engine::WithSources : Engine::AllProducts );
    for( int i = 0; i < ids.size(); i++ )
        res << Product(d_imp->d_eng.data(),ids[i]);
    return res;
}

QList<Module> Module::subModules() const
{
    QList<Module> res;
    if( !isValid() )
        return res;
    QList<int> ids = d_imp->d_eng->getSubModules(d_imp->d_id);
    for( int i = 0; i < ids.size(); i++ )
        res << Module(d_imp->d_eng.data(),ids[i]);
    return res;
}

class Internal::ProjectImp : public QSharedData
{
public:
    QString d_path;
    Engine::Ptr d_eng;
    ErrorInfo d_errs;
    ILogSink* d_log;
    Engine::ParseParams params;
    QProcessEnvironment env;

    ProjectImp():d_log(0){}
    ~ProjectImp()
    {
        if( d_eng.data() )
            d_eng->registerLogger(0,0);
    }

    static inline LoggerLevel map(BSLogLevel ll)
    {
        switch( ll )
        {
        case BS_Info:
            return LoggerTrace;
        case BS_Message:
            return LoggerInfo;
        case BS_Debug:
            return LoggerDebug;
        case BS_Warning:
            return LoggerWarning;
        case BS_Error:
        case BS_Critical:
            return LoggerError;
        }
    }

    static void logger(BSLogLevel ll, void* data, const char* file, BSRowCol loc, const char* format, va_list args)
    {
        ProjectImp* _this = (ProjectImp*)data;
        QByteArray tmp;
        tmp.resize(2000);
        qvsnprintf(tmp.data(),tmp.size(),format,args);
        const QString msg = QString::fromUtf8(tmp);
        LoggerLevel level = map(ll);
        if( ll >= BS_Warning )
        {
            ErrorItem item;
            if( file )
                item.d_loc.d_path = QString::fromUtf8(file);
            if( loc.row )
            {
                item.d_loc.d_row = loc.row;
                item.d_loc.d_col = loc.col + 1;
            }
            item.d_msg = msg;
            if( ll >= BS_Error )
                _this->d_errs.d_errs.append(item);
            if( _this->d_log )
            {
                if( ll == BS_Warning )
                {
                    ErrorInfo ei;
                    ei.d_errs.append(item);
                    _this->d_log->printWarning(ei);
                }
            }
        }
        const QString tag = ILogSink::logLevelTag(level);
        if( _this->d_log && ll != BS_Warning )
            _this->d_log->printMessage(level,msg, tag);

        // qDebug() << (level == LoggerInfo ? "INF" : tag ) << msg; // TEST
    }
};

Project::Project()
{

}

Project::Project(const QString& path)
{
    d_imp = new Internal::ProjectImp();
    if( path.endsWith("BUSY") || path.endsWith("BUSY.busy") )
        d_imp->d_path = QFileInfo(path).absolutePath();
    else
        d_imp->d_path = path;
    d_imp->d_eng = new Engine();
    d_imp->d_eng->registerLogger(Internal::ProjectImp::logger,d_imp.data());
}

Project::Project(const Project& other) : d_imp(other.d_imp)
{

}

Project&Project::operator=(const Project& other)
{
    d_imp = other.d_imp;
    return *this;
}

Project::~Project()
{

}

bool Project::isValid() const
{
    return d_imp->d_eng.constData() != 0;
}

Engine*Project::getEngine() const
{
    return d_imp->d_eng.data();
}

bool Project::parse(const SetupProjectParameters& in, ILogSink* logSink)
{
    if( !isValid() )
        return false;

    ErrorItem err;
    d_imp->d_errs.d_errs.clear();
    d_imp->d_log = logSink;

    d_imp->params.params = in.params;
    d_imp->params.targets = in.targets;
    d_imp->params.root_source_dir = d_imp->d_path.toUtf8();
    d_imp->params.root_build_dir = in.buildDir.toUtf8();

    if( in.buildVariant == "debug" )
        d_imp->params.build_mode = "debug";
    else
        d_imp->params.build_mode = "optimized";

    if( !( in.toolchain == "gcc" || in.toolchain == "clang" || in.toolchain == "msvc" ) )
    {
        err.d_msg = "toolchain not supported: " + in.toolchain;
        d_imp->d_errs.d_errs.append(err);
        return false;
    }else
        d_imp->params.toolchain = in.toolchain.toUtf8();

    QFileInfo info(in.compilerCommand);
    d_imp->params.toolchain_path = info.absolutePath().toUtf8();
    d_imp->params.toolchain_prefix = info.baseName().toUtf8();
    if( d_imp->params.toolchain_prefix.endsWith(".exe") )
        d_imp->params.toolchain_prefix.chop(4);
    if( in.toolchain == "gcc" )
        d_imp->params.toolchain_prefix.chop(3);
    else if( in.toolchain == "clang" )
        d_imp->params.toolchain_prefix.chop(5);
    else if( in.toolchain == "msvc" )
        d_imp->params.toolchain_prefix.chop(2); // cl

    switch(in.abi.architecture())
    {
    case ProjectExplorer::Abi::ArmArchitecture:
        d_imp->params.cpu = "arm";
        break;
    case ProjectExplorer::Abi::X86Architecture:
        d_imp->params.cpu = "x86";
        break;
    case ProjectExplorer::Abi::ItaniumArchitecture:
        d_imp->params.cpu = "ia64";
        break;
    case ProjectExplorer::Abi::MipsArchitecture:
        d_imp->params.cpu = "mips";
        break;
    case ProjectExplorer::Abi::PowerPCArchitecture:
        d_imp->params.cpu = "ppc";
        break;
    default:
        err.d_msg = "toolchain architecture not supported: " + ProjectExplorer::Abi::toString(in.abi.architecture());
        d_imp->d_errs.d_errs.append(err);
        return false;
    }

    switch(in.abi.os())
    {
    case ProjectExplorer::Abi::BsdOS:
        d_imp->params.os = "freebsd";
        break;
    case ProjectExplorer::Abi::LinuxOS:
        d_imp->params.os = "linux";
        break;
    case ProjectExplorer::Abi::MacOS:
        d_imp->params.os = "macos";
        break;
    case ProjectExplorer::Abi::UnixOS:
        d_imp->params.os = "unix";
        break;
    case ProjectExplorer::Abi::WindowsOS:
        d_imp->params.os = "win32";
        break;
    default:
        err.d_msg = "toolchain operating system not supported: " + ProjectExplorer::Abi::toString(in.abi.os());
        d_imp->d_errs.d_errs.append(err);
        return false;
    }

    switch(in.abi.wordWidth())
    {
    case 128:
        d_imp->params.wordsize = "128";
        break;
    case 64:
        d_imp->params.wordsize = "64";
        break;
    case 32:
        d_imp->params.wordsize = "32";
        break;
    case 16:
        d_imp->params.wordsize = "16";
        break;
    default:
        err.d_msg = "toolchain word width not supported: " + ProjectExplorer::Abi::toString(in.abi.wordWidth());
        d_imp->d_errs.d_errs.append(err);
        return false;
    }

    d_imp->env = in.env;

    return d_imp->d_eng->parse(d_imp->params);
}

ErrorInfo Project::errors() const
{
    return d_imp->d_errs;
}

Module Project::topModule() const
{
    if( !isValid() )
        return Module();
    const int ref = d_imp->d_eng->getRootModule();
    if( ref )
        return Module(d_imp->d_eng.data(), ref);
    else
        return Module();
}

class Internal::ProductImp : public QSharedData
{
public:
    Engine::Ptr d_eng;
    int d_id;
};

Product::Product(Engine* eng, int id)
{
    d_imp = new Internal::ProductImp();
    d_imp->d_eng = eng;
    d_imp->d_id = id;
}


Product::Product()
{

}

Product::Product(const Product &other): d_imp(other.d_imp)
{

}

Product &Product::operator=(const Product &other)
{
    d_imp = other.d_imp;
    return *this;
}

bool Product::operator==( const Product& rhs ) const
{
    return isValid() && d_imp->d_id == rhs.d_imp->d_id;
}

Product::~Product()
{
}

bool Product::isValid() const
{
    return d_imp.data() != 0;
}

QString Product::name(bool altName) const
{
    if( !isValid() )
        return QString();
    if( altName )
    {
        const QString name = QString::fromUtf8( d_imp->d_eng->getString(d_imp->d_id,"name",true) );
        if( !name.isEmpty() )
            return name;
    }
    return QString::fromUtf8( d_imp->d_eng->getString(d_imp->d_id,"#name") );
}

QString Product::qualident() const
{
    if( !isValid() )
        return QString();
    return QString::fromUtf8( d_imp->d_eng->getDeclPath(d_imp->d_id));
}

CodeLocation Product::location() const
{
    CodeLocation res;
    if( !isValid() )
        return res;
    const int owner = d_imp->d_eng->getOwner(d_imp->d_id);
    if( owner == 0 )
        return res;
    const QByteArray tmp = d_imp->d_eng->getString(owner,"#file");
    res.d_path = QString::fromUtf8( bs_denormalize_path(tmp.constData()) );
    res.d_col = d_imp->d_eng->getInteger(d_imp->d_id,"#col") + 1;
    res.d_row = d_imp->d_eng->getInteger(d_imp->d_id,"#row");
    return res;
}

CodeLocation Product::endLocation() const
{
    CodeLocation res;
    if( !isValid() )
        return res;
    const int ctx = d_imp->d_eng->getObject(d_imp->d_id, "#ctr");
    if( ctx )
    {
        res.d_path = d_imp->d_eng->getPath(ctx,"#file");
        res.d_col = d_imp->d_eng->getInteger(ctx,"#endcol");
        res.d_row = d_imp->d_eng->getInteger(ctx,"#endrow");
    }
    return res;
}

QList<TargetArtifact> Product::targetArtifacts() const
{
    QList<TargetArtifact> res;
    QString binary = executable(false);
    if( !binary.isEmpty() )
        res << TargetArtifact(binary);
    return res;
}

QVariantMap Product::properties() const
{
    return QVariantMap();
}

bool Product::isEnabled() const
{
    if( !isValid() )
        return false;
    return d_imp->d_eng->isActive(d_imp->d_id);
}

bool Product::isRunnable() const
{
    if( !isValid() )
        return false;
    return d_imp->d_eng->isExecutable(d_imp->d_id);
}

bool Product::isCompiled() const
{
    if( !isValid() )
        return false;
    return d_imp->d_eng->isClass(d_imp->d_id,"CompiledProduct");
}

static QStringList findHeaders(const QStringList& files )
{
    QStringList headers;
    foreach (const QString &file, files) {
        if( file.endsWith(".cpp") || file.endsWith(".c") || file.endsWith(".cc") || file.endsWith(".c")
                || file.endsWith(".c++") || file.endsWith(".cxx")
                || file.endsWith(".m") || file.endsWith(".mm"))
        {
            QFileInfo fi(file);
            const QString base = fi.absoluteDir().absoluteFilePath(fi.completeBaseName() );
            if( QFileInfo(base + ".h").exists() )
                headers << base + ".h";
            else if( QFileInfo(base + ".hh").exists() )
                headers << base + ".hh";
            else if( QFileInfo(base + ".hpp").exists() )
                headers << base + ".hpp";
            else if( QFileInfo(base + ".h++").exists() )
                headers << base + ".h++";
            else if( QFileInfo(base + ".hp").exists() )
                headers << base + ".hp";
            else if( QFileInfo(base + ".hxx").exists() )
                headers << base + ".hxx";

            if( QFileInfo(base + "_p.h").exists() )
                headers << base + "_p.h";
        }
    }
    return headers;
}

QStringList Product::allFilePaths(bool addHeaders, bool addGenerated) const
{
    if( !isValid() )
        return QStringList();
    QStringList sources = d_imp->d_eng->getAllSources(d_imp->d_id, addGenerated);
    if( addHeaders )
        return findHeaders(sources) + sources;
    else
        return sources;
}

PropertyMap Product::buildConfig() const
{
    PropertyMap res;
    if( !isValid() )
        return res;

    res.properties[PropertyMap::INCLUDEPATHS] = d_imp->d_eng->getIncludePaths(d_imp->d_id);
    res.properties[PropertyMap::DEFINES] = d_imp->d_eng->getDefines(d_imp->d_id);
    // TODO: the other properties
    return res;
}

QString Product::executable(bool synthIfEmpty) const
{
    if( !isValid() )
        return QString();
    int id = d_imp->d_eng->getObject(d_imp->d_id,"#inst");
    QString res;
    if( id )
        res = d_imp->d_eng->getPath(id,"#product"); // this only works after build
    if( res.isEmpty() && synthIfEmpty )
    {
        const QString n = name(true);
        id = d_imp->d_eng->getGlobals();
        const QString path = d_imp->d_eng->getPath(id,"root_build_dir");
        id = d_imp->d_eng->getOwner(d_imp->d_id);
        const QString rel = d_imp->d_eng->getPath(id,"#rdir");
        res = QString("%1/%2/%3").arg(path).arg(rel).arg(n);
    }
    return res;
}

static void walkAllModules( const Module& m, QSet<QString>& res )
{
    res << m.busyFile();
    foreach( const Module& sub, m.subModules() )
        walkAllModules(sub, res);
}

QSet<QString> Project::buildSystemFiles() const
{
    QSet<QString> res;
    walkAllModules( topModule(), res );
    return res;
}

QSet<QString> Project::allSources(bool onlyActives, bool addHeaders) const
{
    QList<Product> prods = allProducts(CompiledProducts,onlyActives);
    QSet<QString> res;
    foreach( const Product& p, prods )
    {
        QStringList files = p.allFilePaths();
        if( addHeaders )
            files += findHeaders(files);

        foreach( const QString& f, files)
            res << f;
    }
    return res;
}

QString Project::targetExecutable(const Product& product, const InstallOptions& installoptions) const
{
    return product.executable();
}

QProcessEnvironment Project::getRunEnvironment(const Product& product, const InstallOptions& installOptions, const QProcessEnvironment& environment, Settings* settings) const
{
    return environment;
}

BuildJob* Project::buildAllProducts(const BuildOptions& options, QObject* jobOwner) const
{
    if( !isValid() )
        return 0;

    d_imp->d_errs.d_errs.clear();

    return new BuildJob(jobOwner,d_imp->d_eng.data(),d_imp->env, d_imp->params.targets,
                        options.maxJobCount(), options.d_stopOnError, options.d_trackHeaders);
}

BuildJob*Project::buildSomeProducts(const QList<Product>& products, const BuildOptions& options,
                                    QObject* jobOwner) const
{
    return buildAllProducts(options, jobOwner); // TODO
}

CleanJob*Project::cleanAllProducts(const CleanOptions& options, QObject* jobOwner)
{
    if( !isValid() )
        return 0;
    return new CleanJob(jobOwner,d_imp->d_eng.data(),d_imp->params.targets);
}

InstallJob*Project::installAllProducts(const InstallOptions& options, QObject* jobOwner)
{
    return 0;
}

static QString relativePath( const QDir& dir, const QString& path )
{
    QString res = dir.relativeFilePath(QFileInfo(path).canonicalFilePath());
    if( !res.startsWith(".") )
        res = "./" + res;
    return res;
}

ErrorInfo Project::addFiles(const Product &product, const QStringList &filePaths)
{
    ErrorInfo res;
    const CodeLocation loc = product.endLocation();
    QTextDocument doc;
    QFile f(loc.d_path);
    if( !f.open(QIODevice::ReadOnly) )
    {
        ErrorItem e;
        e.d_loc = loc;
        e.d_msg = "cannot open BUSY file for reading";
        res.d_errs << e;
        return res;
    }
    doc.setPlainText( QString::fromUtf8( f.readAll() ));
    f.close();

    QFileInfo info(loc.d_path);
    QDir module = info.absoluteDir();

    QTextCursor cur(&doc);
    const QTextBlock block = doc.findBlockByNumber(loc.d_row-1);
    cur.setPosition( block.position() + loc.d_col-1 );
    cur.insertText("    .sources += ");
    if( filePaths.size() > 1 )
    {
        cur.insertText( "[\n" );
        foreach( const QString& path, filePaths )
        {
            cur.insertText("        ");
            cur.insertText(relativePath(module,path));
            cur.insertText("\n");
        }
        cur.insertText(QString(loc.d_col-1, QChar(' ') ) );
        cur.insertText( "    ]" );
    }else
        cur.insertText(relativePath(module,filePaths.first()));

    cur.insertText(QString("\n%1").arg(QString(loc.d_col-1, QChar(' ') ) ) );

    if( !f.open(QIODevice::WriteOnly) )
    {
        ErrorItem e;
        e.d_loc = loc;
        e.d_msg = "cannot open BUSY file for writing";
        res.d_errs << e;
        return res;
    }
    f.write( doc.toPlainText().toUtf8() );
    return res;
}

static void walkAllProducts(Engine* eng, int module, QList<int>& res, Engine::ProductFilter filter, bool onlyActives )
{
    QList<int> subs = eng->getSubModules(module);
    for(int i = 0; i < subs.size(); i++ )
        walkAllProducts(eng,subs[i],res,filter, onlyActives);
    res += eng->getAllProducts(module, filter, onlyActives);
}

QList<Product> Project::allProducts(ProductFilter filter, bool onlyActives) const
{
    if( !isValid() )
        return QList<Product>();
    QList<int> ids;

    Engine::ProductFilter ef = Engine::AllProducts;
    switch(filter)
    {
    case AllProducts:
        ef = Engine::WithSources;
        break;
    case RunnableProducts:
        ef = Engine::Executable;
        break;
    case CompiledProducts:
        ef = Engine::Compiled;
        break;
    }

    walkAllProducts(d_imp->d_eng.data(),d_imp->d_eng->getRootModule(),ids, ef, onlyActives);
    QList<Product> res;
    for( int i = 0; i < ids.size(); i++ )
        res << Product(d_imp->d_eng.data(),ids[i]);
    return res;
}

QString ErrorInfo::toString() const
{
    QStringList lines;
    foreach (const ErrorItem &e, d_errs)
    {
        QString str;
        if (!e.codeLocation().filePath().isEmpty()) {
            str = QDir::toNativeSeparators(e.codeLocation().filePath());
            QString lineAndColumn;
            if (e.codeLocation().line() > 0 && !str.contains(QRegExp(QLatin1String(":[0-9]+$"))))
                lineAndColumn += QLatin1Char(':') + QString::number(e.codeLocation().line());
            if (e.codeLocation().column() > 0 && !str.contains(QRegExp(QLatin1String(":[0-9]+:[0-9]+$"))))
                lineAndColumn += QLatin1Char(':') + QString::number(e.codeLocation().column());
            str += lineAndColumn;
        }
        if (!str.isEmpty())
            str += QLatin1Char(' ');
        str += e.description();
        lines.append(str);
    }
    return lines.join(QLatin1Char('\n'));
}

ILogSink::ILogSink():d_level(LoggerInfo)
{

}

ILogSink::~ILogSink()
{

}

void ILogSink::setLogLevel(LoggerLevel level)
{
    d_level = level;
}

bool ILogSink::willPrint(LoggerLevel level) const
{
    return level <= d_level;
}

void ILogSink::printWarning(const ErrorInfo& warning)
{
    if (willPrint(LoggerWarning)) {
        //d->mutex.lock();
        doPrintWarning(warning);
        //d->mutex.unlock();
    }
}

void ILogSink::printMessage(LoggerLevel level, const QString& message, const QString& tag, bool force)
{
    if (force || willPrint(level)) {
        //d->mutex.lock();
        doPrintMessage(level, message, tag);
        //d->mutex.unlock();
    }
}

static QString logLevelName(LoggerLevel level)
{
    switch (level) {
    case LoggerError:
        return QLatin1String("error");
    case LoggerWarning:
        return QLatin1String("warning");
    case LoggerInfo:
        return QLatin1String("info");
    case LoggerDebug:
        return QLatin1String("debug");
    case LoggerTrace:
        return QLatin1String("trace");
    default:
        break;
    }
    return QString();
}

QString ILogSink::logLevelTag(LoggerLevel level)
{
    if (level == LoggerInfo)
        return QString();
    QString str = logLevelName(level).toUpper();
    if (!str.isEmpty())
        str.append(QLatin1String(": "));
    return str;
}

class BuildJob::Imp : public Builder
{
public:
    Imp(int count, bool stopOnErr, bool trackHdr):Builder(count,stopOnErr,trackHdr){}

    QProcessEnvironment env;
    QString workdir;
    QString sourcedir;
    Builder::OpList ops;
};

struct BuildJobVisitorContext
{
    Builder::OpList ops;
    Builder::Operation op;
    quint32 group;
    bool inGroup;
    BuildJobVisitorContext():group(0),inGroup(false){}
#if 0
    QHash<const char*,QByteArray> strings;
    QByteArray toSym(const char* str)
    {
        QByteArray& sym = strings[str];
        if( sym.isEmpty() )
            sym = str;
        return sym;
    }
#else
    QHash<QByteArray,QByteArray> strings;
    QByteArray toSym(const char* str)
    {
        // str pointer is not a reliable identity for the string;
        // it can change over the call of bsvisitor for large projects
        QByteArray tmp(str);
        QByteArray& sym = strings[tmp];
        if( sym.isEmpty() )
            sym = tmp;
        return sym;
    }
#endif
};

extern "C" {
static int BuildJobBeginOp(BSBuildOperation op, const char* command, int toolchain, int os, void* data)
{
    BuildJobVisitorContext* ctx = (BuildJobVisitorContext*)data;

    ctx->op = Builder::Operation();
    ctx->op.op = op;
    ctx->op.os = os;
    ctx->op.tc = toolchain;
    ctx->op.cmd = ctx->toSym(command);
    if( !ctx->inGroup )
        ctx->group++;
    ctx->op.group = ctx->group;

    if( op == BS_EnteringProduct )
        ctx->ops << ctx->op; // no end, so do it here

    return 0;
}

static void BuildJobOpParam(BSBuildParam k, const char* value, void* data)
{
    BuildJobVisitorContext* ctx = (BuildJobVisitorContext*)data;
    Builder::Parameter p;
    p.kind = k;
    p.value = ctx->toSym(value);
    ctx->op.params << p;
}

static void CleanJobOpParam(BSBuildParam k, const char* value, void* data)
{
    QList<QString>* files = (QList<QString> *)data;
    if( k == BS_outfile )
        files->append( QString::fromUtf8(value) );
}

static void BuildJobEndOp(void* data)
{
    BuildJobVisitorContext* ctx = (BuildJobVisitorContext*)data;
    ctx->ops << ctx->op;
}

static void BuildJobForkGroup(int n, void* data)
{
    BuildJobVisitorContext* ctx = (BuildJobVisitorContext*)data;

    if( n > 0)
        ctx->group++;
    ctx->inGroup = n >= 0;
}
}

static void dumpOps( const Builder::OpList& ops )
{
    QFile f( QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("ops.log") );
    f.open(QIODevice::WriteOnly);
    QTextStream out(&f);

    for( int i = 0; i < ops.size(); i++ )
    {
        QByteArray prefix;
        switch(ops[i].op)
        {
        case BS_Compile:
            prefix = "COMPILE: ";
            break;
        case BS_LinkExe:
        case BS_LinkDll:
        case BS_LinkLib:
            prefix = "LINK: ";
            break;
        case BS_RunMoc:
            prefix = "MOC: ";
            break;
        case BS_RunRcc:
            prefix = "RCC: ";
            break;
        case BS_RunUic:
            prefix = "UIC: ";
            break;
        case BS_RunLua:
            prefix = "LUA: ";
            break;
        case BS_Copy:
            prefix = "COPY: ";
            break;
        case BS_EnteringProduct:
            prefix = "### running " + ops[i].cmd + ": ";
            break;
        default:
            prefix = "BEGIN OP: " + QByteArray::number(ops[i].op) + " ";
            break;
        }
        out << i << " " << prefix.constData() << " " << ops[i].group << " " << ops[i].getOutfile() << endl;

#if 1
        for( int j = 0; j < ops[i].params.size(); j++ )
        {
            switch(ops[i].params[j].kind)
            {
            case BS_infile:
                prefix = "  INFILE: ";
                break;
            case BS_outfile:
                prefix = "  OUTFILE: ";
                break;
            case BS_cflag:
                prefix = "  CFLAG: ";
                break;
            case BS_define:
                prefix = "  DEFINE: ";
                break;
            case BS_include_dir:
                prefix = "  INCLUDEDIR: ";
                break;
            case BS_ldflag:
                prefix = "  LDFLAG: ";
                break;
            case BS_lib_dir:
                prefix = "  LIBDIR: ";
                break;
            case BS_lib_name:
                prefix = "  LIBNAME: ";
                break;
            default:
                prefix = "  PARAM: ";
                break;
            }
            out << prefix.constData() << " " << ops[i].params[j].value.constData() << endl;
        }
#endif
    }
}

BuildJob::BuildJob(QObject* owner, Engine* eng, const QProcessEnvironment& env,
                   const QByteArrayList& targets, int count, bool stopOnErr, bool trackHdr)
    :AbstractJob(owner)
{
    eng->createBuildDirs();

    BuildJobVisitorContext ctx;
    const bool res = eng->visit(BuildJobBeginOp,BuildJobOpParam,BuildJobEndOp,BuildJobForkGroup, &ctx, targets);

#if 0
    dumpOps(ctx.ops);
#endif

    d_imp = new Imp(count, stopOnErr, trackHdr);
    d_imp->env = env;
    const int globals = eng->getGlobals();
    d_imp->workdir = eng->getPath(globals,"root_build_dir");
    d_imp->sourcedir = eng->getPath(globals,"root_source_dir");
    d_imp->ops = ctx.ops;

    connect(d_imp,SIGNAL(taskStarted(const QString&,int)),this,SIGNAL(taskStarted(const QString&,int)));
    connect(d_imp,SIGNAL(taskProgress(int)),this,SIGNAL(taskProgress(int)));
    connect(d_imp,SIGNAL(taskFinished(bool)),this,SIGNAL(taskFinished(bool)));
    connect(d_imp,SIGNAL(reportCommandDescription(const QString&, const QString&)),
            this,SIGNAL(reportCommandDescription(const QString&, const QString&)));
    connect(d_imp,SIGNAL(reportResult( bool, const QStringList& )),
            this,SLOT(reportResult( bool, const QStringList& )));
}

BuildJob::~BuildJob()
{
    d_imp->deleteLater();
}

void BuildJob::start()
{
    d_imp->start( d_imp->ops, d_imp->sourcedir, d_imp->workdir, d_imp->env );
}

void BuildJob::cancel()
{
    QMetaObject::invokeMethod( d_imp, "onCancel" );
}

void BuildJob::reportResult( bool success, const QStringList& stdErr )
{
    ProcessResult res;
    res.stdErr = stdErr;
    res.success = success;
    res.workingDirectory = d_imp->workdir;
    qRegisterMetaType<ProcessResult>();
    emit reportProcessResult(res);
}

int BuildOptions::defaultMaxJobCount()
{
    const int count = QThread::idealThreadCount();
    return qMax(count,1);
}


CleanJob::CleanJob(QObject* owner, Engine* eng, const QByteArrayList& targets):AbstractJob(owner)
{
    eng->visit(0,CleanJobOpParam,0,0, &d_files, targets);
}

void CleanJob::start()
{
    emit taskStarted("Cleaning BUSY project", d_files.size() );
    for( int i = 0; i < d_files.size(); i++ )
    {
        QFile::remove(d_files[i]);
        emit taskProgress(i);
    }
    emit taskFinished(true);
}
