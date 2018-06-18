import pprint
import os
import ast
import sys
from textwrap import dedent

from ir import *


# see http://dev.stephendiehl.com/numpile/
def ast2tree(node, include_attrs=True):
    def _transform(node):
        if isinstance(node, ast.AST):
            fields = ((a, _transform(b))
                      for a, b in ast.iter_fields(node))
            if include_attrs:
                attrs = ((a, _transform(getattr(node, a)))
                         for a in node._attributes
                         if hasattr(node, a))
                return (node.__class__.__name__, dict(fields), dict(attrs))
            return (node.__class__.__name__, dict(fields))
        elif isinstance(node, list):
            return [_transform(x) for x in node]
        elif isinstance(node, str):
            return repr(node)
        return node
    if not isinstance(node, ast.AST):
        raise TypeError('expected AST, got %r' % node.__class__.__name__)
    return _transform(node)


def pformat_ast(node, include_attrs=False, **kws):
    return pprint.pformat(ast2tree(node, include_attrs), **kws)


class cg1Node(ast.AST):
    def build(self, builder):
        pass
        # raise NotImplementedError(self)

class cg1CodeNode(cg1Node):
    pass
    # def load(self, ctx):
        # return self.build(ctx)


class cg1DeclareVar(cg1Node):
    _fields = ["name", "type"]

    def __init__(self, name="<anon>", type="i32"):
        self.name = name
        self.type = type

    # def build(self, ctx):
    #     assert self.type == 'i32'

    #     ins = ctx['builder'].alloca(llvmtype_i32, size=1, name=self.name)
    #     ctx['locals'][ctx['func']][self.name] = ins

    #     return ins


class cg1Var(cg1Node):
    _fields = ["name", "type"]

    def __init__(self, name="<anon>"):
        self.name = name
        self.type = None

    # def lookup(self, ctx):
    #     # this sets the lookup precedence between locals and globals
    #     try:
    #         return ctx['locals'][ctx['func']][self.name]

    #     except KeyError:
    #         return ctx['globals'][self.name]

    # def load(self, ctx):
    #    return self.build(ctx)

    def __str__(self):
        return "cg1Var %s %s" % (self.name, self.type)


class cg1ObjVar(cg1Var):
    def __init__(self, *args, **kwargs):
        super(cg1ObjVar, self).__init__(*args, **kwargs)
        self.type = 'obj'

        toks = self.name.split('.')
        self.obj = toks[0]
        self.attr = toks[1]

    # def lookup(self, ctx):
    #     return ctx['objects'][self.obj][self.attr]

    # def load(self, ctx):
    #     var = self.lookup(ctx)

    #     return ctx['builder'].load(var)

    # def build(self, ctx):
    #     return self.lookup(ctx)

class cg1LocalVar(cg1Var):
    pass

class cg1VarInt32(cg1Var):
    def __init__(self, *args, **kwargs):
        super(cg1VarInt32, self).__init__(*args, **kwargs)
        self.type = 'i32'

    # def load(self, ctx):
    #     var = self.lookup(ctx)

    #     if isinstance(var, ir.Argument):
    #         return var

    #     return ctx['builder'].load(var)

    # def build(self, ctx):
    #     var = self.lookup(ctx)
    #     return var
       
class cg1ConstInt32(cg1Var):
    pass
    # def build(self, ctx):
    #     if self.name not in ctx['constants']:
    #         ctx['constants'][self.name] = ir.Constant(llvmtype_i32, self.name)

    #     return ctx['constants'][self.name]


class cg1Module(cg1Node):
    _fields = ["name", "body"]

    def __init__(self, name, body):
        self.name = name
        self.body = body
        self.module = None
        self.ctx = {}

    def build(self, builder=None):
        if builder == None:
            builder = IRBuilder()

        # collect everything at module level that is not part of a function
        startup_code = [a for a in self.body if not isinstance(a, cg1Func)]

        for node in startup_code:
            # assign global vars to table
            if isinstance(node, cg1DeclareVar):
                builder.add_global(node.name, node.type, 1)

        # collect funcs
        funcs = [a for a in self.body if isinstance(a, cg1Func)]

        for code in funcs:
            code.build(builder)

        return builder



    # def build(self):
    #     module = ir.Module(name=self.name)

    #     ctx = {'builder': None, 
    #            'functions': {}, 
    #            'func': None, 
    #            'locals': {}, 
    #            'globals':{}, 
    #            'constants':{}, 
    #            'objects': {},
    #            'module': module}

    #     ctx['builder'] = ir.IRBuilder()
    #     self.ctx = ctx

    #     ctx['objects']['pixels'] = {
    #         'hue': ir.GlobalVariable(ctx['module'], llvmtype_i32, 'pixels.hue'),
    #         'sat': ir.GlobalVariable(ctx['module'], llvmtype_i32, 'pixels.sat'),
    #         'val': ir.GlobalVariable(ctx['module'], llvmtype_i32, 'pixels.val')}

    #     # collect everything at module level that is not part of a function
    #     startup_code = [a for a in self.body if not isinstance(a, cg1Func)]

    #     for node in startup_code:
    #         # assign global vars to table
    #         if isinstance(node, cg1DeclareVar):
    #             ctx['globals'][node.name] = ir.GlobalVariable(ctx['module'], llvmtype_i32, str(node.name))

    #     # collect funcs
    #     funcs = [a for a in self.body if isinstance(a, cg1Func)]

    #     # if len(startup_code) > 0:
    #     #     startup_func = cg1Func("__global_init", [], startup_code)
    #     #     startup_func.build(ctx)

    #     for code in funcs:
    #         code.build(ctx)

    #     del ctx['func']
    #     del ctx['module']
    #     del ctx['builder']
    #     self.module = module
    #     return self

class cg1NoOp(cg1CodeNode):
    def build(self, ctx):
        pass

class cg1Func(cg1CodeNode):
    _fields = ["name", "params", "body"]

    def __init__(self, name, params, body):
        self.name = name
        self.params = params
        self.body = body

    # def build(self, ctx):
    #     func_type = ir.FunctionType(llvmtype_i32, [llvmtype_i32 for a in self.params])

    #     func = ir.Function(ctx['module'], func_type, name=self.name)
    #     block = func.append_basic_block(name="entry")
    #     ctx['builder'] = ir.IRBuilder(block)
    #     ctx['functions'][self.name] = func
    #     ctx['func'] = self.name
    #     ctx['locals'][ctx['func']] = {}
        
    #     for (param, arg) in zip(self.params, func.args):
    #         ctx['locals'][ctx['func']][param.name] = arg

    #     for node in self.body:
    #         node.build(ctx)

    #     # check if we need a default return
    #     if not isinstance(self.body[-1], cg1Return):
    #         ret = cg1Return(cg1ConstInt32(0))
    #         ret.build(ctx)

class cg1Return(cg1CodeNode):
    _fields = ["value"]

    def __init__(self, value):
        self.value = value

    # def build(self, ctx):
        # return ctx['builder'].ret(self.value.build(ctx))

class cg1Call(cg1CodeNode):
    _fields = ["target", "params"]

    def __init__(self, target, params):
        self.target = target
        self.params = params

    # def build(self, ctx):
    #     params = [a.load(ctx) for a in self.params]

    #     return ctx['builder'].call(ctx['functions'][self.target], params)

class cg1Assign(cg1CodeNode):
    _fields = ["target", "value"]

    def __init__(self, target, value):
        self.target = target
        self.value = value

    # def build(self, ctx):
    #     value = self.value.build(ctx)
    #     target = self.target.build(ctx)

    #     if isinstance(value, ir.AllocaInstr):
    #         value = self.value.load(ctx)

    #     elif isinstance(value, ir.GlobalVariable):
    #         value = self.value.load(ctx)

    #     return ctx['builder'].store(value, target)

class cg1If(cg1CodeNode):
    _fields = ["test", "body", "orelse"]

    def __init__(self, test, body, orelse):
        self.test = test
        self.body = body
        self.orelse = orelse

    # def build(self, ctx):
    #     test = self.test.build(ctx)

    #     with ctx['builder'].if_else(test) as (then, otherwise):
    #         with then:
    #             for a in self.body:
    #                 a.build(ctx)

    #         with otherwise:
    #             for a in self.orelse:
    #                 a.build(ctx)


class cg1BinOpNode(cg1CodeNode):
    _fields = ["op", "left", "right"]

    def __init__(self, op, left, right):
        self.op = op
        self.left = left
        self.right = right

    # def build(self, ctx):
    #     left = self.left.load(ctx)
    #     right = self.right.load(ctx)

    #     if self.op == "add":
    #         result = ctx['builder'].add(left, right)

    #     elif self.op == "lt":
    #         result = ctx['builder'].icmp_signed("<", left, right)

    #     else:
    #         raise NotImplementedError

    #     return result

class cg1CompareNode(cg1BinOpNode):
    pass
        

class cg1For(cg1CodeNode):
    _fields = ["target", "iter", "body"]

    def __init__(self, target, iter, body):
        self.target = target
        self.iter = iter
        self.body = body

    # def build(self, ctx):
    #     func = ctx['functions'][ctx['func']]
    #     init_block = func.append_basic_block('for.init')
    #     test_block = func.append_basic_block('for.test')
    #     body_block = func.append_basic_block('for.body')
    #     end_block = func.append_basic_block("for.end")

    #     # init iterator local variable and set to 0
    #     ctx['builder'].branch(init_block)
    #     ctx['builder'].position_at_end(init_block)
    #     declare_target = cg1DeclareVar(self.target.name)
    #     target = declare_target.build(ctx)
    #     ctx['builder'].store(cg1ConstInt32(0).build(ctx), target)

        
    #     # set up condition test
    #     ctx['builder'].branch(test_block)
    #     ctx['builder'].position_at_end(test_block)

    #     compare = cg1CompareNode('lt', self.target, self.iter)
    #     cond = compare.build(ctx)
        
    #     # conditional branch
    #     ctx['builder'].cbranch(cond, body_block, end_block)

    #     # loop body
    #     ctx['builder'].position_at_end(body_block)

    #     for a in self.body:
    #         a.build(ctx)

    #     # increment loop counter
    #     inc = ctx['builder'].add(cg1ConstInt32(1).build(ctx), ctx['builder'].load(target))
    #     ctx['builder'].store(inc, target)

    #     # branch back to top of loop
    #     ctx['builder'].branch(test_block)

    #     ctx['builder'].position_at_end(end_block)


class CodeGenPass1(ast.NodeVisitor):
    def __init__(self):
        pass

    def __call__(self, source):
        # remove leading indentation
        source = dedent(source)

        self._source = source
        self._ast = ast.parse(source)

        return self.visit(self._ast)

    def visit_Module(self, node):
        body = map(self.visit, node.body)
        
        return cg1Module("module", body)

    def visit_FunctionDef(self, node):
        body = map(self.visit, list(node.body))
        params = map(self.visit, node.args.args)
        return cg1Func(node.name, params, body)

    def visit_Return(self, node):
        return cg1Return(self.visit(node.value))

    def visit_Call(self, node):
        if node.func.id == "Number":
            return cg1DeclareVar(type="i32")

        else:
            return cg1Call(node.func.id, map(self.visit, node.args))

    def visit_If(self, node):
        return cg1If(self.visit(node.test), map(self.visit, node.body), map(self.visit, node.orelse))
    
    def visit_Compare(self, node):
        assert len(node.ops) <= 1

        left = self.visit(node.left)
        right = self.visit(node.comparators[0])
        op = self.visit(node.ops[0])

        return cg1CompareNode(op, left, right)

    def visit_Assign(self, node):
        assert len(node.targets) == 1

        target = self.visit(node.targets[0])
        value = self.visit(node.value)

        if isinstance(value, cg1DeclareVar):
            value.name = target.name
            return value

        else:
            return cg1Assign(target, value)

    def visit_AugAssign(self, node):
        binop = cg1BinOpNode(self.visit(node.op), self.visit(node.target), self.visit(node.value))
        
        return cg1Assign(self.visit(node.target), binop)

    def visit_Num(self, node):
        if isinstance(node.n, int):
            return cg1ConstInt32(node.n)

        elif isinstance(node.n, float):
            # convert to int
            return cg1ConstInt32(int(node.n * 65535))

        else:
            raise NotImplementedError(node)    
    
    def visit_Name(self, node):
        return cg1VarInt32(node.id)

    def visit_BinOp(self, node):
        return cg1BinOpNode(self.visit(node.op), self.visit(node.left), self.visit(node.right))

    def visit_Add(self, node):
        return "add"

    def visit_Lt(self, node):
        return "lt"

    def visit_Expr(self, node):
        return self.visit(node.value)

    def visit_For(self, node):
        # Check for an else clause.  Python has an atypical else construct
        # you can use at the end of a for loop.  But it is confusing and
        # rarely used, so we are not going to support it.
        assert len(node.orelse) == 0

        return cg1For(self.visit(node.target), self.visit(node.iter), map(self.visit, node.body))

    def visit_Attribute(self, node):
        name = '%s.%s' % (node.value.id, node.attr)

        return cg1ObjVar(name)

    def visit_Pass(self, node):
        return cg1NoOp()

    def generic_visit(self, node):
        raise NotImplementedError(node)




with open('cg2_test.fx') as f:
    source = f.read()


# with open('rainbow2.fx') as f:
    # source = f.read()

tree = ast.parse(source)

print pformat_ast(tree)

print '\n'

cg1 = CodeGenPass1()

cg1_data = cg1(source)

print pformat_ast(cg1_data)


builder = cg1_data.build()

print builder



