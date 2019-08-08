i = Number()
a = Number()
b = Number()

def init():
    while i < 0:
        for x in pixels.size_x:
            a += 1

    for y in pixels.size_x:
        b -= 2

        if b < 0:
            break

        elif b == 0:
            pass
            
        else:
            b += 1


# # a = String(publish=True)
# b = String(publish=True)
# c = String(32, publish=True)

# def init():
#     # a = "test"
#     s = String('test2')
#     b = s
#     # c = a
#     b = c

#     db.vm_prog = s


# my_str = String("rainbow.fxb")
# s2 = String(publish=True)
# s3 = String(publish=True)

# def init():
#     # s = String()
#     s_meow = String('meow')

#     # s = my_str
#     # s2 = my_str
#     # pass

#     # s2 = "meow"
#     s2 = my_str
#     s3 = "meow"