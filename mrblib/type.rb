module MTypeInf
  class BasicType
    UNDEF_VALUE = [:undef]
    def initialize(co, *rest)
      @class_object = co
    end

    def ==(other)
      self.class == other.class &&
      @class_object == other.class_object
    end

    def type_equal(other, tup)
      self == other
    end

    attr :class_object

    def merge(arr)
      if @class_object != Class || 1 then
        arr.each_with_index do |ele, i|
          if ele.class_object == @class_object then
            if ele.is_a?(MTypeInf::PrimitiveType) then
              return

            elsif is_a?(MTypeInf::PrimitiveType) then
              arr[i] = MTypeInf::PrimitiveType.new(ele.class_object)
              return
            end
          end

          if ele == self then
            case ele
            when MTypeInf::LiteralType
              if ele.val != @val then
                if ele.class_object != Class  then
                  arr[i] = MTypeInf::PrimitiveType.new(ele.class_object)
                  return
                end

              else
                return
              end

              when MTypeInf::ContainerType
                if ele.element[UNDEF_VALUE] == @element[UNDEF_VALUE] then
                  return
                end

              when MTypeInf::UserDefinedType,
                MTypeInf::PrimitiveType,
                MTypeInf::ProcType
                return

              else
                raise self

              end
          end

          #        elsif ele < self then
          #
          #        end
        end
      end

      arr.push self
    end

    def inspect_aux(level)
      @class_object.inspect
    end

    def inspect
      inspect_aux(0)
    end

    def inspecto_element
      inspect
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
      reg = RiteSSA::Reg.new(nil)
#      reg.add_type PrimitiveType.new(NilClass, nil), 0
      @element[UNDEF_VALUE] = reg
    end

    def ==(other)
#      self.class == other.class &&
#        @class_object == other.class_object &&
#        @element == other.element
      equal?(other)
    end

    def type_equal(other, tup)
      if self.class != other.class ||
          @class_object != other.class_object then
        return false
      end
      return other.element[UNDEF_VALUE] == @element[nil]
    end

    def inspect_aux(level)
      if level > 5 then
        return "..."
      end

      elearr = []
      @element.values.each do |ele|
        ele.type.values.each do |tys|
          tys.each do |ty|
            elearr << ty.inspect_aux(level + 1)
          end
        end
      end
      "#{@class_object.inspect}<#{elearr.uniq.join('|')}>"
    end

    def inspecto
      res = "<#{@class_object} element=["
      @element.each do |key, val|
        res << "#{key.inspect}="
        val.type.each do |tup, tys|
          res << "#{tup} = ["
          tys.each do |ty|
            res << "#{ty.inspect_element}|"
          end
          res << "]\n"
        end
        res << "\n"
      end
      res << "]>"
      res
    end

    def inspect_element
      "<#{@class_object} element=...>"
    end

    attr :element
  end

  class ProcType<BasicType
    def initialize(co, irep, slf, env, *rest)
      super
      @irep = irep
      @slf = slf
      @env = env
    end

    def ==(other)
      if self.class == other.class &&
          @class_object == other.class_object &&
          @irep == other.irep then
        if @env != other.env
          false
        else
          true
        end
      else
        false
      end
    end

    def inspect
      "#{@class_object.inspect}<irep=#{@irep.irep.id.to_s(16)} env=#{env}>"
    end

    attr :irep
    attr :slf
    attr :env
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
