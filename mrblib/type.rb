module MTypeInf
  class BasicType
    def initialize(co, *rest)
      @class_object = co
    end

    def ==(other)
      self.class == other.class &&
      @class_object == other.class_object
    end

    attr :class_object

    def merge(arr)
      arr.each_with_index do |ele, i|
        if ele.class_object == @class_object and
            ele.is_a?(MTypeInf::PrimitiveType) then
          return
        end

        if ele.class_object == @class_object and
            is_a?(MTypeInf::PrimitiveType) then
          arr[i] = MTypeInf::PrimitiveType.new(ele.class_object)
          return
        end

        if ele == self then
          case ele
          when MTypeInf::LiteralType
            if ele.val != @val then
              arr[i] = MTypeInf::PrimitiveType.new(ele.class_object)
            end

            return

          when MTypeInf::ContainerType
            # TODO Merge element types

            return

          when MTypeInf::UserDefinedType, MTypeInf::PrimitiveType
            return

          when MTypeInf::ProcType
            if ele.irep == @irep then
              return
            end
          end
        end

#        elsif ele < self then
#
#        end
      end
      arr.push self
    end
  end

  class PrimitiveType<BasicType
  end

  class LiteralType<BasicType
    def initialize(co, val, *rest)
      super
      @val = val
    end

    attr :val
  end

  class ContainerType<BasicType
    def initialize(co, *rest)
      super
      @element = {}
      @element[nil] = []
    end

    attr :element
  end

  class ProcType<BasicType
    def initialize(co, irep, *rest)
      super
      @irep = irep
    end

    def ==(other)
      self.class == other.class &&
        @class_object == other.class_object &&
        @irep == other.irep
    end

    attr :irep
  end

  class UserDefinedType<BasicType
    @@class_tab = {}

    def initialize(co, *rest)
      super
    end
  end

  TypeSource = {
    NilClass => PrimitiveType,
    Fixnum => PrimitiveType,
    Float => PrimitiveType,
    Symbol => PrimitiveType,
    TrueClass => PrimitiveType,
    FalseClass => PrimitiveType,
    String => PrimitiveType,

    Array => ContainerType,
    Hash => ContainerType,
    Range => ContainerType,
  }
  TypeTable = {}
  TypeSource.each do |ty, cl|
    TypeTable[ty] = cl.new(ty)
  end
end
