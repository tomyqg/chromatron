# <license>
# 
#     This file is part of the Sapphire Operating System.
# 
#     Copyright (C) 2013-2018  Jeremy Billheimer
# 
# 
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
# 
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
# </license>

from instructions import *

from copy import deepcopy, copy

ARRAY_FUNCS = ['len', 'min', 'max', 'avg', 'sum']


class SyntaxError(Exception):
    def __init__(self, message='', lineno=None):
        self.lineno = lineno

        message += ' (line %d)' % (self.lineno)

        super(SyntaxError, self).__init__(message)

class VMRuntimeError(Exception):
    def __init__(self, message=''):
        super(VMRuntimeError, self).__init__(message)


def params_to_string(params):
    s = ''

    for p in params:
        s += '%s %s,' % (p.type, p.name)

    # strip last comma
    s = s[:len(s) - 1]

    return s

class IR(object):
    def __init__(self, lineno=None):
        self.lineno = lineno

        assert self.lineno != None

    def generate(self):
        return BaseInstruction()

class irVar(IR):
    def __init__(self, name, type='i32', **kwargs):
        super(irVar, self).__init__(**kwargs)
        self.name = name
        self.type = type
        self.length = 1
        self.addr = None

    def __str__(self):
        return "Var (%s, %s)" % (self.name, self.type)

    def generate(self):
        assert self.addr != None
        return insAddr(self.addr, self)

    def lookup(self, indexes):
        assert len(indexes) == 0
        return copy(self)

    def get_base_type(self):
        return self.type

class irVar_i32(irVar):
    def __init__(self, *args, **kwargs):
        super(irVar_i32, self).__init__(*args, **kwargs)
        self.type = 'i32'

class irVar_f16(irVar):
    def __init__(self, *args, **kwargs):
        super(irVar_f16, self).__init__(*args, **kwargs)
        self.type = 'f16'

class irVar_gfx16(irVar):
    def __init__(self, *args, **kwargs):
        super(irVar_gfx16, self).__init__(*args, **kwargs)
        self.type = 'gfx16'

class irAddress(irVar):
    def __init__(self, name, target=None, **kwargs):
        super(irAddress, self).__init__(name, **kwargs)
        self.target = target
        self.type = 'addr_i32'

    def __str__(self):
        return "Addr (%s -> %s)" % (self.name, self.target)

    def generate(self):
        assert self.addr != None
        return insAddr(self.addr, self.target)

    def get_base_type(self):
        return self.target.get_base_type()

class irConst(irVar):
    def __str__(self):
        value = self.name
        if self.type == 'f16':
            # convert internal to float for printing
            value = (self.name >> 16) + (self.name & 0xffff) / 65536.0

        return "Const (%s, %s)" % (value, self.type)

class irArray(irVar):
    def __init__(self, name, type, dimensions=[], **kwargs):
        super(irArray, self).__init__(name, type, **kwargs)

        self.count = dimensions.pop(0)

        if len(dimensions) == 0:
            self.type = type

        else:
            self.type = irArray(name, type, dimensions, lineno=self.lineno)

        self.length = self.count * self.type.length

    def __str__(self):
        return "Array (%s, %s, %d:%d)" % (self.name, self.type, self.count, self.length)

    def lookup(self, indexes):
        if len(indexes) == 0:
            return copy(self)

        indexes = deepcopy(indexes)
        indexes.pop(0)
        
        return self.type.lookup(indexes)

    def get_base_type(self):
        data_type = self.type

        while isinstance(data_type, irVar):
            data_type = data_type.type

        return data_type

class irRecord(irVar):
    def __init__(self, name, data_type, fields, offsets, **kwargs):
        super(irRecord, self).__init__(name, **kwargs)        

        self.fields = fields
        self.type = data_type

        self.offsets = offsets
        self.length = 0
        for field in self.fields.values():
            self.length += field.length


        self.count = 0
            
    def __call__(self, name, dimensions=[], lineno=None):
        return irRecord(name, self.type, self.fields, self.offsets, lineno=lineno)

    def __str__(self):
        return "Record (%s, %s, %d)" % (self.name, self.type, self.length)

    def get_field_from_offset(self, offset): 
        for field_name, addr in self.offsets.items():
            if addr.name == offset.name:
                return self.fields[field_name]

        assert False

    def lookup(self, indexes):
        if len(indexes) == 0:
            return copy(self)

        indexes = deepcopy(indexes)
        index = indexes.pop(0)

        try:
            return self.fields[index.name].lookup(indexes)

        except KeyError:
            # try looking up by offset
            for field_name, addr in self.offsets.items():
                if addr.name == index.name:
                    return self.fields[field_name].lookup(indexes)

            raise

class irStr(IR):
    def __init__(self, name, **kwargs):
        super(irStr, self).__init__(**kwargs)
        self.name = name
        
    def __str__(self):
        return "Str(%s)" % (self.name)

    def generate(self):
        return self.name

class irField(IR):
    def __init__(self, name, obj, **kwargs):
        super(irField, self).__init__(**kwargs)
        self.name = name
        self.obj = obj
        
    def __str__(self):
        return "Field (%s.%s)" % (self.obj.name, self.name)

    def generate(self):
        return insAddr(self.obj.offsets[self.name].addr)

class irObject(IR):
    def __init__(self, name, data_type, args=[], kw={}, **kwargs):
        super(irObject, self).__init__(**kwargs)
        self.name = name
        self.type = data_type
        self.args = args
        self.kw = kw

    def __str__(self):
        return "Object %s(%s)" % (self.name, self.type)

    def get_base_type(self):
        return self.type

class irPixelArray(irObject):
    def __init__(self, name, args=[], kw={}, **kwargs):
        super(irPixelArray, self).__init__(name, "PixelArray", args, kw, **kwargs)
        # super(irPixelArray, self).__init__(name, "gfx16", args, kw, **kwargs)
        
    def __str__(self):
        return "PixelArray %s" % (self.name)

    def lookup(self, indexes):
        return self

class irObjectAttr(irAddress):
    def __init__(self, obj, attr, **kwargs):
        super(irObjectAttr, self).__init__(obj.name, target=attr, **kwargs)      

        self.obj = obj
        self.attr = attr.name
        self.type = obj.type

    def __str__(self):
        return "ObjAttr (%s.%s)" % (self.name, self.attr)


PIX_ATTRS = {
    'hue': 0,
    'sat': 1,
    'val': 2,
    'hs_fade': 3,
    'v_fade': 4,
    'count': 5,
    'size_x': 6,
    'size_y': 7,
    'index': 8,
}

class irPixelAttr(irObjectAttr):
    def __init__(self, obj, attr, **kwargs):
        lineno = kwargs['lineno']

        # attr = irVar_gfx16(attr, lineno=lineno)
        attr = irArray(attr, irVar_gfx16(attr, lineno=lineno), dimensions=[65535, 65535], lineno=lineno)

        super(irPixelAttr, self).__init__(obj, attr, **kwargs)      

        self.addr = 65535

    def __str__(self):
        return "PixelAttr (%s.%s)" % (self.name, self.attr)

    def generate(self):
        return self

class irFunc(IR):
    def __init__(self, name, ret_type='i32', params=None, body=None, **kwargs):
        super(irFunc, self).__init__(**kwargs)
        self.name = name
        self.ret_type = ret_type
        self.params = params
        self.body = body

        if self.params == None:
            self.params = []

        if self.body == None:
            self.body = []

    def append(self, node):
        self.body.append(node)

    def __str__(self):
        params = params_to_string(self.params)

        s = "Func %s(%s) -> %s\n" % (self.name, params, self.ret_type)

        for node in self.body:
            s += '%d\t\t%s\n' % (node.lineno, node)

        return s

    def generate(self):
        params = [a.generate() for a in self.params]
        func = insFunction(self.name, params)
        ins = [func]
        for ir in self.body:
            code = ir.generate()

            try:
                ins.extend(code)

            except TypeError:
                ins.append(code)

        return ins

class irReturn(IR):
    def __init__(self, ret_var, **kwargs):
        super(irReturn, self).__init__(**kwargs)
        self.ret_var = ret_var

    def __str__(self):
        return "RET %s" % (self.ret_var)

    def generate(self):
        return insReturn(self.ret_var.generate())

class irNop(IR):
    def __str__(self, **kwargs):
        return "NOP" 

    def generate(self):
        return insNop()

class irBinop(IR):
    def __init__(self, result, op, left, right, **kwargs):
        super(irBinop, self).__init__(**kwargs)
        self.result = result
        self.op = op
        self.left = left
        self.right = right
    
    def __str__(self):
        s = '%s = %s %s %s' % (self.result, self.left, self.op, self.right)

        return s

    def generate(self):
        ops = {
            'i32':
                {'eq': insCompareEq,
                'neq': insCompareNeq,
                'gt': insCompareGt,
                'gte': insCompareGtE,
                'lt': insCompareLt,
                'lte': insCompareLtE,
                'logical_and': insAnd,
                'logical_or': insOr,
                'add': insAdd,
                'sub': insSub,
                'mul': insMul,
                'div': insDiv,
                'mod': insMod},
            'f16':
                {'eq': insF16CompareEq,
                'neq': insF16CompareNeq,
                'gt': insF16CompareGt,
                'gte': insF16CompareGtE,
                'lt': insF16CompareLt,
                'lte': insF16CompareLtE,
                'logical_and': insF16And,
                'logical_or': insF16Or,
                'add': insF16Add,
                'sub': insF16Sub,
                'mul': insF16Mul,
                'div': insF16Div,
                'mod': insF16Mod},
        }
        
        return ops[self.result.type][self.op](self.result.generate(), self.left.generate(), self.right.generate())


class irUnaryNot(IR):
    def __init__(self, target, value, **kwargs):
        super(irUnaryNot, self).__init__(**kwargs)
        self.target = target
        self.value = value

    def __str__(self):
        return "%s = NOT %s" % (self.target, self.value)

    def generate(self):
        return insNot(self.target.generate(), self.value.generate())



type_conversions = {
    ('i32', 'f16'): insConvF16toI32,
    ('f16', 'i32'): insConvI32toF16,
    ('i32', 'gfx16'): insConvMov,
    ('gfx16', 'f16'): insConvMov,
    ('gfx16', 'i32'): insConvMov,
}
        
# convert value to result's type and store in result
class irConvertType(IR):
    def __init__(self, result, value, **kwargs):
        super(irConvertType, self).__init__(**kwargs)
        self.result = result
        self.value = value

        assert self.result.type != self.value.type
    
    def __str__(self):
        s = '%s = %s(%s)' % (self.result, self.result.type, self.value)

        return s

    def generate(self):
        return type_conversions[(self.result.type, self.value.type)](self.result.generate(), self.value.generate())

class irConvertTypeInPlace(IR):
    def __init__(self, target, src_type, **kwargs):
        super(irConvertTypeInPlace, self).__init__(**kwargs)
        self.target = target
        self.src_type = src_type
    
    def __str__(self):
        s = '%s = %s(%s)' % (self.target, self.target.type, self.target)

        return s

    def generate(self):
        return type_conversions[(self.target.type, self.src_type)](self.target.generate(), self.target.generate())


class irVectorOp(IR):
    def __init__(self, op, target, value, **kwargs):
        super(irVectorOp, self).__init__(**kwargs)
        self.op = op
        self.target = target
        self.value = value
        
    def __str__(self):
        s = '*%s %s=(vector) %s' % (self.target, self.op, self.value)

        return s

    def generate(self):
        ops = {
            'add': insVectorAdd,
            'sub': insVectorSub,
            'mul': insVectorMul,
            'div': insVectorDiv,
            'mod': insVectorMod,
        }

        pixel_ops = {
            'add': insPixelVectorAdd,
            'sub': insPixelVectorSub,
            'mul': insPixelVectorMul,
            'div': insPixelVectorDiv,
            'mod': insPixelVectorMod,
        }

        if isinstance(self.target, irPixelAttr):
            target = self.target.generate()
            return pixel_ops[self.op](target.name, target.attr, self.value.generate())

        else:
            return ops[self.op](self.target.generate(), self.value.generate())

class irAssign(IR):
    def __init__(self, target, value, **kwargs):
        super(irAssign, self).__init__(**kwargs)
        self.target = target
        self.value = value

        assert self.target.length == 1
        
    def __str__(self):
        return '%s = %s' % (self.target, self.value)

    def generate(self):
        return insMov(self.target.generate(), self.value.generate())

class irVectorAssign(IR):
    def __init__(self, target, value, **kwargs):
        super(irVectorAssign, self).__init__(**kwargs)
        self.target = target
        self.value = value
        
    def __str__(self):
        return '*%s =(vector) %s' % (self.target, self.value)

    def generate(self):
        if isinstance(self.target, irPixelAttr):
            target = self.target.generate()
            return insPixelVectorMov(target.name, target.attr, self.value.generate())

        else:
            return insVectorMov(self.target.generate(), self.value.generate())

class irCall(IR):
    def __init__(self, target, params, args, result, **kwargs):
        super(irCall, self).__init__(**kwargs)
        self.target = target
        self.params = params
        self.args = args
        self.result = result

    def __str__(self):
        params = params_to_string(self.params)
        s = 'CALL %s(%s) -> %s' % (self.target, params, self.result)

        return s

    def generate(self):        
        params = [a.generate() for a in self.params]
        args = [a.generate() for a in self.args]

        # call func
        call_ins = insCall(self.target, params, args)

        # move return value to result register
        mov_ins = insMov(self.result.generate(), insAddr(0))

        return [call_ins, mov_ins]

class irLibCall(IR):
    def __init__(self, target, params, result, **kwargs):
        super(irLibCall, self).__init__(**kwargs)
        self.target = target
        self.params = params
        self.result = result

    def __str__(self):
        params = params_to_string(self.params)
        s = 'LCALL %s(%s) -> %s' % (self.target, params, self.result)

        return s

    def generate(self):        
        params = [a.generate() for a in self.params]

        if self.target in ARRAY_FUNCS:
            if len(params) != 1:
                raise SyntaxError("Array functions take one argument", lineno=self.lineno)

        # call func
        call_ins = insLibCall(self.target, self.result, params)

        return call_ins


class irLabel(IR):
    def __init__(self, name, **kwargs):
        super(irLabel, self).__init__(**kwargs)        
        self.name = name

    def __str__(self):
        s = 'LABEL %s' % (self.name)

        return s

    def generate(self):
        return insLabel(self.name)


class irBranchConditional(IR):
    def __init__(self, value, target, **kwargs):
        super(irBranchConditional, self).__init__(**kwargs)        
        self.value = value
        self.target = target

class irBranchZero(irBranchConditional):
    def __init__(self, *args, **kwargs):
        super(irBranchZero, self).__init__(*args, **kwargs)        

    def __str__(self):
        s = 'BR Z %s -> %s' % (self.value, self.target.name)

        return s    

    def generate(self):
        return insJmpIfZero(self.value.generate(), self.target.generate())
        


class irBranchNotZero(irBranchConditional):
    def __init__(self, *args, **kwargs):
        super(irBranchNotZero, self).__init__(*args, **kwargs)        

    def __str__(self):
        s = 'BR NZ %s -> %s' % (self.value, self.target.name)

        return s    

    def generate(self):
        return insJmpIfNotZero(self.value.generate(), self.target.generate())
    

class irJump(IR):
    def __init__(self, target, **kwargs):
        super(irJump, self).__init__(**kwargs)        
        self.target = target

    def __str__(self):
        s = 'JMP -> %s' % (self.target.name)

        return s    

    def generate(self):
        return insJmp(self.target.generate())
    

class irJumpLessPreInc(IR):
    def __init__(self, target, op1, op2, **kwargs):
        super(irJumpLessPreInc, self).__init__(**kwargs)        
        self.target = target
        self.op1 = op1
        self.op2 = op2

    def __str__(self):
        s = 'JMP (%s++) < %s -> %s' % (self.op1, self.op2, self.target.name)

        return s    

    def generate(self):
        return insJmpIfLessThanPreInc(self.op1.generate(), self.op2.generate(), self.target.generate())

class irAssert(IR):
    def __init__(self, value, **kwargs):
        super(irAssert, self).__init__(**kwargs)        
        self.value = value

    def __str__(self):
        s = 'ASSERT %s' % (self.value)

        return s   


class irIndex(IR):
    def __init__(self, result, target, indexes=[], **kwargs):
        super(irIndex, self).__init__(**kwargs)        
        self.result = result
        self.target = target
        self.indexes = indexes

    def __str__(self):
        indexes = ''
        for index in self.indexes:
            indexes += '[%s]' % (index.name)

        s = '%s = INDEX %s%s' % (self.result, self.target, indexes)

        return s

    def generate(self):
        indexes = [i.generate() for i in self.indexes]
        counts = []
        strides = []

        target = self.target

        for i in xrange(len(self.indexes)):
            count = target.count
            counts.append(count)
            
            if isinstance(target, irRecord):
                target = target.get_field_from_offset(self.indexes[i])
                stride = 0

            else:
                target = target.type
                stride = target.length

            strides.append(stride)
        

        return insIndex(self.result.generate(), self.target.generate(), indexes, counts, strides)


class irPixelIndex(IR):
    def __init__(self, target, indexes=[], **kwargs):
        super(irPixelIndex, self).__init__(**kwargs)        

        self.name = target.name
        self.target = target
        self.indexes = indexes
        self.attr = None

    def __str__(self):
        indexes = ''
        for index in self.indexes:
            indexes += '[%s]' % (index.name)

        s = 'PIXEL INDEX %s.%s%s' % (self.name, self.attr, indexes)

        return s

    def get_base_type(self):
        return 'gfx16'


class irPixelStore(IR):
    def __init__(self, target, value, **kwargs):
        super(irPixelStore, self).__init__(**kwargs)
        self.target = target
        self.value = value
        
    def __str__(self):
        indexes = ''
        for index in self.target.indexes:
            indexes += '[%s]' % (index.name)

        return '%s.%s%s = %s' % (self.target.name, self.target.attr, indexes, self.value)

    def generate(self):
        return insPixelStore(self.target.name, self.target.attr, self.target.indexes, self.value.generate())


class irIndexLoad(IR):
    def __init__(self, result, address, **kwargs):
        super(irIndexLoad, self).__init__(**kwargs)        
        self.result = result
        self.address = address

    def __str__(self):
        s = '%s = *%s' % (self.result, self.address)

        return s    

    def generate(self):
        return insIndirectLoad(self.result.generate(), self.address.generate())

class irIndexStore(IR):
    def __init__(self, address, value, **kwargs):
        super(irIndexStore, self).__init__(**kwargs)        
        self.address = address
        self.value = value

    def __str__(self):
        s = '*%s = %s' % (self.address, self.value)

        return s    

    def generate(self):
        return insIndirectStore(self.value.generate(), self.address.generate())


class Builder(object):
    def __init__(self):
        self.funcs = {}
        self.locals = {}
        self.globals = {}
        self.objects = {}
        self.pixel_arrays = {}
        self.labels = {}

        self.data_table = []

        self.loop_top = []
        self.loop_end = []

        self.next_temp = 0

        self.compound_lookup = []

        self.current_func = None

        self.data_types = {
            'i32': irVar_i32,
            'f16': irVar_f16,
            'gfx16': irVar_gfx16,
            'addr': irAddress,
        }

        self.record_types = {}

        # optimizations
        self.optimizations = {
            'fold_constants': False
        }

        # make sure we always have 0 const
        self.add_const(0, lineno=0)

        # self.add_global('pixels.sat', data_type='gfx16', dimensions=[1], lineno=0)
        # self.add_global('pixels.val', data_type='gfx16', dimensions=[1], lineno=0)
        # self.add_global('pixels.hue', data_type='gfx16', dimensions=[1], lineno=0)

        # create main pixels object
        self.pixelarray_object('pixels', args=[0, 65535], lineno=0)

    def __str__(self):
        s = "FX IR:\n"

        s += 'Globals:\n'
        for i in self.globals.values():
            s += '%d\t%s\n' % (i.lineno, i)

        s += 'Locals:\n'
        for fname in sorted(self.locals.keys()):
            if len(self.locals[fname].values()) > 0:
                s += '\t%s\n' % (fname)

                for l in sorted(self.locals[fname].values()):
                    s += '%d\t\t%s\n' % (l.lineno, l)

        s += 'PixelArrays:\n'
        for i in self.pixel_arrays.values():
            s += '%d\t%s\n' % (i.lineno, i)

        s += 'Functions:\n'
        for func in self.funcs.values():
            s += '%d\t%s\n' % (func.lineno, func)

        return s

    def add_type(self, name, data_type, lineno=None):
        if name in self.data_types:
            raise SyntaxError("Type '%s' already defined" % (name), lineno=lineno)

        self.data_types[name] = data_type

    def create_record(self, name, fields, lineno=None):
        new_fields = {}
        offsets = {}
        offset = 0
        for field_name, field in fields.items():
            new_fields[field_name] = self.build_var(field_name, field['type'], field['dimensions'], lineno=lineno)

            offsets[field_name] = self.add_const(offset, lineno=lineno)
            offset += new_fields[field_name].length

        ir = irRecord(name, name, new_fields, offsets, lineno=lineno)

        self.record_types[name] = ir

    def get_type(self, name, lineno=None):
        if name not in self.data_types:

            # try records
            if name not in self.record_types:
                raise SyntaxError("Type '%s' not defined" % (name), lineno=lineno)

            return self.record_types[name]

        return self.data_types[name]

    def build_var(self, name, data_type, dimensions=[], lineno=None):
        data_type = self.get_type(data_type, lineno=lineno)

        if len(dimensions) == 0:
            ir = data_type(name, lineno=lineno)

        else:
            ir = irArray(name, data_type(name, lineno=lineno), dimensions=dimensions, lineno=lineno)

        return ir

    def add_global(self, name, data_type='i32', dimensions=[], lineno=None):
        if name in self.globals:
            return self.globals[name]

        ir = self.build_var(name, data_type, dimensions, lineno=lineno)
        
        try:   
            for v in ir:
                self.globals[v.name] = v

        except TypeError:
            self.globals[name] = ir

        return ir

    def add_local(self, name, data_type='i32', dimensions=[], lineno=None):
        # check if this is already in the globals
        if name in self.globals:
            return self.globals[name]

        if name in self.locals[self.current_func]:
            return self.locals[self.current_func][name]

        ir = self.build_var(name, data_type, dimensions, lineno=lineno)

        try:
            for v in ir:
                self.locals[self.current_func][v.name] = v

        except TypeError:
            self.locals[self.current_func][name] = ir

        return ir

    def get_var(self, name, lineno=None):
        if name in self.pixel_arrays:
            return self.pixel_arrays[name]

        if name in self.globals:
            return self.globals[name]

        try:
            return self.locals[self.current_func][name]

        except KeyError:
            raise SyntaxError("Variable '%s' not declared" % (name), lineno=lineno)

    def get_obj_var(self, obj_name, attr, lineno=None):
        if obj_name in self.pixel_arrays:
            obj = self.pixel_arrays[obj_name]

            ir = irPixelAttr(obj, attr, lineno=lineno)

            return ir

        else:
            raise SyntaxError("Object '%s' not declared" % (obj_name), lineno=lineno)            

    def add_const(self, name, data_type='i32', lineno=None):
        if name in self.globals:
            return self.globals[name]

        ir = irConst(name, data_type, lineno=lineno)

        self.globals[name] = ir

        return ir

    def add_temp(self, data_type='i32', lineno=None):
        name = '%' + str(self.next_temp)
        self.next_temp += 1

        ir = self.build_var(name, data_type, [], lineno=lineno)
        self.locals[self.current_func][name] = ir

        return ir

    def add_func_arg(self, func, arg):
        # if arg.name in self.globals:
            # raise SyntaxError("Argument name '%s' already declared as global" % (arg.name), lineno=func.lineno)

        arg.name = '$%s.%s' % (func.name, arg.name)

        func.params.append(arg)

    def func(self, *args, **kwargs):
        func = irFunc(*args, **kwargs)
        self.funcs[func.name] = func
        self.locals[func.name] = {}
        self.current_func = func.name
        self.next_temp = 0 

        return func

    def append_node(self, node):
        self.funcs[self.current_func].append(node)

    def ret(self, value, lineno=None):
        ir = irReturn(value, lineno=lineno)

        self.append_node(ir)

        return ir

    def nop(self, lineno=None):
        ir = irNop(lineno=lineno)

        self.append_node(ir)

        return ir

    def binop(self, op, left, right, lineno=None):
        if self.optimizations['fold_constants'] and \
            isinstance(left, irConst) and \
            isinstance(right, irConst):

            return self._fold_constants(op, left, right, lineno)

        # if either type is fixed16, we do the whole thing as fixed16.
        data_type = left.type
        if right.type == 'f16':
            data_type = right.type

        left_result = left
        right_result = right
        
        # perform any conversions as needed
        # since we are prioritizing fixed16, we only need to convert i32 to f16
        if data_type == 'f16':
            if left.type != 'f16':
                left_result = self.add_temp(data_type=data_type, lineno=lineno)

                ir = irConvertType(left_result, left, lineno=lineno)
                self.append_node(ir)

            if right.type != 'f16':
                right_result = self.add_temp(data_type=data_type, lineno=lineno)

                ir = irConvertType(right_result, right, lineno=lineno)
                self.append_node(ir)

        # generate result register with target data type
        result = self.add_temp(data_type=data_type, lineno=lineno)

        ir = irBinop(result, op, left_result, right_result, lineno=lineno)

        self.append_node(ir)

        return result

    def unary_not(self, value, lineno=None):
        # generate result register with target data type
        result = self.add_temp(data_type=value.type, lineno=lineno)
    
        ir = irUnaryNot(result, value, lineno=lineno)        

        self.append_node(ir)

        return result

    def assign(self, target, value, lineno=None):   
        # check types
        # don't do conversion if value is an address
        if target.get_base_type() != value.get_base_type() and not isinstance(value, irAddress):
            # in normal expressions, f16 will take precedence over i32.
            # however, for the assign, the assignment target will 
            # have priority.

            # convert value to target type and replace value with result
            conv_result = self.add_temp(lineno=lineno, data_type=target.get_base_type())
            ir = irConvertType(conv_result, value, lineno=lineno)
            self.append_node(ir)
            value = conv_result

        if isinstance(value, irAddress):
            if value.target.length > 1:
                raise SyntaxError("Cannot assign from compound type '%s' to '%s'" % (value.target.name, target.name), lineno=lineno)

            self.load_indirect(value, target, lineno=lineno)

            # check types
            if target.get_base_type() != value.get_base_type():
                # mismatch.
                # in this case, we've already done the indirect load into the target, but 
                # it has the wrong type. we're going to do the conversion on top of itself.
                ir = irConvertTypeInPlace(target, value.get_base_type(), lineno=lineno)
                self.append_node(ir)

        elif isinstance(target, irAddress):
            if target.target.length == 1:
                self.store_indirect(target, value, lineno=lineno)

            else:
                ir = irVectorAssign(target, value, lineno=lineno)
                self.append_node(ir)

        elif isinstance(target, irArray):
            result = self.add_temp(lineno=lineno, data_type='addr')
            ir = irIndex(result, target, lineno=lineno)
            self.append_node(ir)
            result.target = target

            ir = irVectorAssign(result, value, lineno=lineno)
            self.append_node(ir)

        elif isinstance(target, irPixelIndex):
            ir = irPixelStore(target, value, lineno=lineno)
            self.append_node(ir)

        else:
            ir = irAssign(target, value, lineno=lineno)
            self.append_node(ir)

    def augassign(self, op, target, value, lineno=None):
        # check types
        if target.get_base_type() != value.get_base_type():
            # in normal expressions, f16 will take precedence over i32.
            # however, for the augassign, the assignment target will 
            # have priority.

            # convert value to target type and replace value with result
            conv_result = self.add_temp(lineno=lineno, data_type=target.get_base_type())
            ir = irConvertType(conv_result, value, lineno=lineno)
            self.append_node(ir)
            value = conv_result

        if isinstance(target, irAddress):
            if target.target.length == 1:
                # not a vector op, but we have a target address and not a value
    
                # need to load indirect first
                result = self.load_indirect(target, lineno=lineno)
                result = self.binop(op, result, value, lineno=lineno)

                self.assign(target, result, lineno=lineno)

            else:
                result = target
                ir = irVectorOp(op, result, value, lineno=lineno)        
                self.append_node(ir)

        elif target.length == 1:
            # if so, we can replace with a binop and assign
            result = self.binop(op, target, value, lineno=lineno)
            self.assign(target, result, lineno=lineno)

        else:
            # index address of target
            result = self.add_temp(lineno=lineno, data_type='addr')
            ir = irIndex(result, target, lineno=lineno)
            self.append_node(ir)
            result.target = target

            ir = irVectorOp(op, result, value, lineno=lineno)        
            self.append_node(ir)

    def load_indirect(self, address, result=None, lineno=None):
        if result is None:
            result = self.add_temp(data_type=address.target.type, lineno=lineno)

        ir = irIndexLoad(result, address, lineno=lineno)
    
        self.append_node(ir)

        return result  

    def store_indirect(self, address, value, lineno=None):
        ir = irIndexStore(address, value, lineno=lineno)
    
        self.append_node(ir)

        return ir
        
    def call(self, func_name, params, lineno=None):
        result = self.add_temp(lineno=lineno)

        try:
            args = self.funcs[func_name].params
            ir = irCall(func_name, params, args, result, lineno=lineno)

        except KeyError:
            ir = irLibCall(func_name, params, result, lineno=lineno)
        
        self.append_node(ir)        

        return result

    def label(self, name, lineno=None):
        if name not in self.labels:
            self.labels[name] = 0

        else:
            self.labels[name] += 1

        name += '.%d' % (self.labels[name])

        ir = irLabel(name, lineno=lineno)
        return ir

    def ifelse(self, test, lineno=None):
        body_label = self.label('if.then', lineno=lineno)
        else_label = self.label('if.else', lineno=lineno)
        end_label = self.label('if.end', lineno=lineno)

        branch = irBranchZero(test, else_label, lineno=lineno)
        self.append_node(branch)

        return body_label, else_label, end_label

    def position_label(self, label):
        self.append_node(label)

    def begin_while(self, lineno=None):
        top_label = self.label('while.top', lineno=lineno)
        end_label = self.label('while.end', lineno=lineno)
        self.position_label(top_label)

        self.loop_top.append(top_label)
        self.loop_end.append(end_label)

    def test_while(self, test, lineno=None):
        ir = irBranchZero(test, self.loop_end[-1], lineno=lineno)
        self.append_node(ir)

    def end_while(self, lineno=None):
        ir = irJump(self.loop_top[-1], lineno=lineno)
        self.append_node(ir)

        self.position_label(self.loop_end[-1])

        self.loop_top.pop(-1)
        self.loop_end.pop(-1)

    def begin_for(self, iterator, lineno=None):
        begin_label = self.label('for.begin', lineno=lineno) # we don't actually need this label, but it is helpful for reading the IR
        self.position_label(begin_label)
        top_label = self.label('for.top', lineno=lineno)
        continue_label = self.label('for.cont', lineno=lineno)
        end_label = self.label('for.end', lineno=lineno)

        self.loop_top.append(continue_label)
        self.loop_end.append(end_label)

        # set up iterator code (init to -1, as first pass will increment before the body) 
        init_value = self.add_const(-1, lineno=lineno)
        ir = irAssign(iterator, init_value, lineno=lineno)
        self.append_node(ir)

        ir = irJump(continue_label, lineno=lineno)
        self.append_node(ir)

        return top_label, continue_label, end_label

    def end_for(self, iterator, stop, top, lineno=None):
        if stop.length != 1:
            raise SyntaxError("Invalid loop iteration count for '%s'" % (stop.name), lineno=lineno)

        ir = irJumpLessPreInc(top, iterator, stop, lineno=lineno)
        self.append_node(ir)

        self.loop_top.pop(-1)
        self.loop_end.pop(-1)

    def jump(self, target, lineno=None):
        ir = irJump(target, lineno=lineno)
        self.append_node(ir)

    def loop_break(self, lineno=None):
        assert self.loop_end[-1] != None
        self.jump(self.loop_end[-1], lineno=lineno)


    def loop_continue(self, lineno=None):
        assert self.loop_top[-1] != None    
        self.jump(self.loop_top[-1], lineno=lineno)

    def assertion(self, test, lineno=None):
        ir = irAssert(test, lineno=lineno)

        self.append_node(ir)

    # def lookup_attribute(self, obj, attr, lineno=None):
    #     if len(self.compound_lookup) == 0:
    #         self.compound_lookup.append(obj)  

    #     self.compound_lookup.append(irField(attr, obj, lineno=lineno))

    def lookup_subscript(self, target, index, lineno=None):
        if len(self.compound_lookup) == 0:
            self.compound_lookup.append(target)

        if isinstance(index, irStr):
            resolved_target = target.lookup(self.compound_lookup[1:])

            if not isinstance(resolved_target, irRecord):
                raise SyntaxError("Invalid index: %s" % (index.name), lineno=lineno)

            # convert index to offset adress for record
            try:
                index = resolved_target.offsets[index.name]

            except KeyError:
                raise SyntaxError("Field '%s' not found in '%s'" % (index.name, target.name), lineno=lineno)
            
        self.compound_lookup.append(index)

    def resolve_lookup(self, lineno=None):    
        target = self.compound_lookup.pop(0)

        indexes = []
        for index in self.compound_lookup:
            indexes.append(index)
        
        self.compound_lookup = []


        if isinstance(target, irPixelArray):
            return irPixelIndex(target, indexes, lineno=lineno)

        else:
            result = self.add_temp(lineno=lineno, data_type='addr')

            result.target = target.lookup(self.compound_lookup)

            ir = irIndex(result, target, indexes, lineno=lineno)

            self.append_node(ir)

            return result

    def pixelarray_object(self, name, args=[], kw={}, lineno=None):    
        if name in self.pixel_arrays:
            raise SyntaxError("PixelArray '%s' already defined" % (name), lineno=lineno)

        self.pixel_arrays[name] = irPixelArray(name, args, kw, lineno=lineno)

    def generic_object(self, name, data_type, args=[], kw={}, lineno=None):
        if data_type == 'PixelArray':
            self.pixelarray_object(name, args, kw, lineno=lineno)

            return

        if name in self.objects:
            raise SyntaxError("Object '%s' already defined" % (name), lineno=lineno)

        self.objects[name] = irObject(name, data_type, args, kw, lineno=lineno)

    def _fold_constants(self, op, left, right, lineno):
        val = None

        if op == 'eq':
            val = left.name == right.name

        elif op == 'neq':
            val = left.name != right.name

        elif op == 'gt':
            val = left.name > right.name

        elif op == 'gte':
            val = left.name >= right.name

        elif op == 'lt':
            val = left.name < right.name

        elif op == 'lte':
            val = left.name <= right.name

        elif op == 'logical_and':
            val = left.name and right.name

        elif op == 'logical_or':
            val = left.name or right.name

        elif op == 'add':
            val = left.name + right.name

        elif op == 'sub':
            val = left.name - right.name

        elif op == 'mult':
            val = left.name * right.name

        elif op == 'div':
            val = left.name / right.name

        elif op == 'mod':
            val = left.name % right.name

        assert val != None

        # make sure we only emit integers
        return self.add_const(int(val), lineno=lineno)

    def allocate(self):
        self.data_table = []

        ret_var = irVar('$return', lineno=0)
        ret_var.addr = 0

        self.data_table.append(ret_var)

        addr = 1
        for i in self.globals.values():
            i.addr = addr
            addr += i.length

            self.data_table.append(i)

        for func_name, local in self.locals.items():
            for i in local.values():
                i.addr = addr
                addr += i.length

                # assign func name to var
                i.name = '%s.%s' % (func_name, i.name)

                self.data_table.append(i)

        return self.data_table

    def print_data_table(self, data):
        print "DATA: "
        for i in data:
            print '\t%3d: %s' % (i.addr, i)

    def print_instructions(self, instructions):
        print "INSTRUCTIONS: "
        i = 0
        for ins in instructions:
            s = '\t%3d: %s' % (i, str(ins))
            print s
            i += 1

    def generate_instructions(self):
        # check if there is no init function
        if 'init' not in self.funcs:
            self.func('init', lineno=0)
            self.ret(self.get_var(0), lineno=0)

        # check if there is no loop function
        if 'loop' not in self.funcs:
            self.func('loop', lineno=0)
            self.ret(self.get_var(0), lineno=0)

        ins = []

        for func in self.funcs.values():
            ins.extend(func.generate())


        return ins



class VM(object):
    def __init__(self, code, data, pix_size_x=4, pix_size_y=4):
        self.code = code
        self.data = data

        # init memory
        self.memory = []

        for var in data:
            if isinstance(var, irConst):
                self.memory.append(var.name)

            else:
                for i in xrange(var.length):
                    self.memory.append(0)

    def dump_registers(self):
        registers = {}
        for var in self.data:
            # don't dump consts
            if isinstance(var, irConst):
                continue

            if isinstance(var, irArray):
                # print var
                value = []
                addr = var.addr
                for i in xrange(var.length):
                    value.append(self.memory[addr])
                    addr += 1

            elif isinstance(var, irRecord):
                value = {}

                for f in var.fields:
                    index = self.memory[var.offsets[f].addr]
                    value[f] = self.memory[index + var.addr]

            else:
                value = self.memory[var.addr]

            # convert fixed16 to float
            if var.type == 'f16':
                value = (value >> 16) + (value & 0xffff) / 65536.0

            registers[var.name] = value

        return registers

    def run_once(self):
        self.run('init')
        self.run('loop')

    def run(self, func):
        cycles = 0
        pc = 0

        return_stack = []

        offsets = {}

        # scan code stream and get offsets for all functions and labels
        for i in xrange(len(self.code)):
            ins = self.code[i]
            if isinstance(ins, insFunction) or isinstance(ins, insLabel):
                offsets[ins.name] = i


        # setup PC
        try:
            pc = offsets[func]
        
        except KeyError:
            raise VMRuntimeError("Function '%s' not found" % (func))

        while True:
            cycles += 1

            ins = self.code[pc]

            # print cycles, pc, ins

            pc += 1

            try:
                ret_val = ins.execute(self.memory)

                if isinstance(ins, insCall):
                    # push PC to return stack
                    return_stack.append(pc)

                # if returning label to jump to
                if ret_val != None:
                    # jump to target
                    pc = offsets[ret_val.name]
                
            except ReturnException:
                if len(return_stack) == 0:
                    # program is complete
                    break

                # pop PC from return stack
                pc = return_stack.pop(-1)


        # clean up





