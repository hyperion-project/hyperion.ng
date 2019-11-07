<template>
  <div
    :class="[baseClass,computedContainerClass]"
    style="width:200px; height:180px; margin:10px; padding:5px 10px; background-color:#2b819a"
    @click="goTo($withBase(to))"
  >
    <div style="width:100%">
      <h3>{{title}}</h3>
    </div>
    <div class="sub-text flex flex-center">
      <p>{{text}}</p>
    </div>
  </div>
</template>

<script>
export default {
  props: ["title", "text", "to", "disabled"],
  data() {
    return {
      baseClass: "card text-center cursor-pointer dark"
    };
  },
  methods: {},
  computed: {
    computedContainerClass() {
      return this.disabled == "" ? "main-section-disabled" : "";
    }
  },
  methods: {
    goTo(val) {
      if (this.disabled != "") {
        document.location.href = val;
      }
    }
  }
};
</script>

<style>
.card {
  box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2);
  transition: 0.3s;
}

.card:hover {
  box-shadow: 0 8px 16px 0 rgba(0, 0, 0, 0.2);
}
</style>